local AiService = class()
local CompoundActionFactory = require 'services.server.ai.compound_action_factory'
local placeholders = require 'services.server.ai.placeholders'
local log = radiant.log.create_logger('ai.service')

function AiService:initialize()
   -- SUSPEND_THREAD is a unique, non-integer token which indicates the thread
   -- should suspend.  It must be non-intenger, as yielding an int means "wait
   -- until this time".  By creating a table, we guarantee the value of
   -- SUSPEND_THREAD is unique when compared with ==.  The name is for
   -- cosmetic purposes and aids in debugging.
   self.SUSPEND_THREAD = { name = "SUSPEND_THREAD" }
   self.KILL_THREAD = { name = "KILL_THREAD" }
   self.RESERVATION_LEASE_NAME = 'ai_reservation'

   for name, value in pairs(placeholders) do
      AiService[name] = value
   end
   AiService.ANY = { ANY = 'Any lua value will do' }
   AiService.NIL = { NIL = 'The nil value' }

   if not self._sv._initialized then
      self._sv._initialized = true
      self.__saved_variables:mark_changed()
      self:_trace_entity_creation()
   else
      -- WARNING: entities created in by other listeners of game_loaded will have a race with this method
      --
      -- Restoring ai's in kind of a pain because actions should be re-injected, but observers should not
      -- (as observers are controllers, have their own state, and restore themselves). There were a few options
      -- considered for restoring ai's on load:
      --
      -- 1) Save the ai_injector for the base ui someplace persistent. It will correctly restore itself on load.
      -- Then we trace new entities' post_create after game_loaded to inject their base ai.
      -- 
      -- 2) Don't save the ai_injector for the base ui. We trace post_create for all entities on service
      -- initialization and tell the ai_injector not to inject observers for post_create events that occur
      -- prior to game_loaded. After game_loaded the ai_injector should resume injecting observers as usual.
      --
      -- 3) We require any entity with an action or observer to define another equipment entity which has
      -- an equipment_piece_component that holds the ai.
      --
      -- Option 1 is currently what is implemented. Maybe someone has a better idea.
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
            self:_trace_entity_creation()
         end)
   end
end

function AiService:destroy()
   if self._entity_post_create_trace then
      self._entity_post_create_trace:destroy()
      self._entity_post_create_trace = nil
   end
end

function AiService:_trace_entity_creation()
   -- Entities without ai_components may still have observers in their ai_packs entity data.
   -- Handle those (and even the ones with ai_components, actually), here.
   self._entity_post_create_trace = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         local ai_packs = radiant.entities.get_entity_data(entity, 'stonehearth:ai_packs')
         if ai_packs and ai_packs.packs then
            self:inject_ai_packs(entity, ai_packs.packs, true)
         end
      end)
end

function AiService:inject_ai_packs(entity, packs, permanent)
   if permanent == nil then
      permanent = false
   end

   local ai = { ai_packs = packs}
   return self:inject_ai(entity, ai, permanent)
end

function AiService:inject_ai(entity, ai, permanent)
   if permanent == nil then
      permanent = false
   end

   local ai_injector = radiant.create_controller('stonehearth:ai_injector', entity, ai)

   if permanent then
      local aic = entity:add_component('stonehearth:ai')
      aic:add_permanent_ai(ai_injector)
      return nil
   else
      return ai_injector
   end
end

function AiService:format_activity(activity)
   return activity.name .. '(' .. self:format_args(activity.args) .. ')'
end

function AiService:format_args(args)
   local msg = ''
   if args then
      assert(type(args) == 'table', string.format('invalid activity arguments: %s', radiant.util.tostring(args)))
      for name, value in pairs(args) do
         if #msg > 0 then
            msg = msg .. ', '
         end
         msg = msg .. string.format('%s = %s ', name, radiant.util.tostring(value))
      end
   end
   return '{ ' .. msg .. '}'
end

function AiService:create_compound_action(action_ctor)
   assert(action_ctor)
   return CompoundActionFactory(action_ctor)
end

function AiService:acquire_ai_lease(target, owner, options)
   if target and target:is_valid() and owner and owner:is_valid() then
      local options = options or {
         persistent = false,
      }
      return target:add_component('stonehearth:lease'):acquire(self.RESERVATION_LEASE_NAME, owner, options)
   end
   return false
end

function AiService:get_ai_lease_owner(target)
   local lc = target:get_component('stonehearth:lease')
   return lc and lc:get_owner(stonehearth.ai.RESERVATION_LEASE_NAME, target)
end

function AiService:release_ai_lease(target, owner)
   if target and target:is_valid() and owner and owner:is_valid() then
      target:add_component('stonehearth:lease'):release(self.RESERVATION_LEASE_NAME, owner)
   end
end

function AiService:can_acquire_ai_lease(target, owner)
   if target and target:is_valid() then
      return target:add_component('stonehearth:lease'):can_acquire(self.RESERVATION_LEASE_NAME, owner)
   end
end


function AiService:create_activity(name, args)
   if args == nil then
      args = {}
   end
   assert(type(name) == 'string', 'activity name must be a string')
   assert(type(args) == 'table', 'activity arguments must be an associative array')
   assert(not args[1], 'activity arguments contains numeric elements (invalid!)')

   -- common error... trying to pass a class instance (e.g. 'foo', { my_instance })
   -- this won't catch them all, but will catch all uses of unclasslib clases
   assert(not args.__class, 'attempt to pass class for activity args (not using associative array?)')
   assert(not args.__type,  'attempt to pass instance for activity args (not using associative array?)')

   return {
      name = name,
      args = args
   }
end

local LAST_ID = 0
function AiService:get_next_object_id()
   LAST_ID = LAST_ID + 1
   return LAST_ID
end

-- because of subtle races in the correctness of ai.CURRENT.carrying, we may get to a point
-- in a pickup action where ai.CURRENT.carrying was nil in start_thinking(), but we actually
-- end up carrying something in the action.  if we abort(), we may loop around and just do
-- the same thing again (!!!), so instead do our best to clear the carry before running the
-- pickup.  again, this only happens if we lose the race.  most of the time, it's a nop. - tony
--
function AiService:prepare_to_pickup_item(ai, entity, item)
   local carrying = radiant.entities.get_carrying(entity)
   if carrying == item then
      ai:get_log():debug('already carrying %s!  early exit out of run', tostring(item))
      return true
   end
   if carrying ~= nil then
      ai:get_log():debug('unexpectedly found ourselves carrying %s just before pickup.  clearing', carrying)
      ai:execute('stonehearth:clear_carrying_now')
   end
   return false
end

-- used from inside an ai action to pickup an item.  because of subtle races in the ai
-- system pre-emption code, an action which wants to pick something up might actually
-- get run while an entity is already carrying something.  in these cases, we need to
-- take some additional precautions before trying to pick something up.  these may include
-- moving what the entity is carrying into their backpack or dropping it on the ground.
-- `pickup_item` will handle all those details for you.
--
function AiService:pickup_item(ai, entity, item)
   -- make sure we are called on the ai thread, since we may need to do things like
   -- drop the carried item (which may play an animation).
   assert(entity:get_component('stonehearth:ai'):get_thread():is_running())


   if stonehearth.ai:prepare_to_pickup_item(ai, entity, item) then
      -- we're already carrying the thing you wanted.  just return it!
      return item
   end
   assert(not radiant.entities.get_carrying(entity))

   -- our carryblock is empty.  go ahead and pick up
   item = radiant.entities.pickup_item(entity, item)
   if not item then
      ai:abort('failed to move item to carry block')
   end
   return item
end

return AiService
