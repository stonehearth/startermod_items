local AiService = class()
local AiInjector = require 'services.server.ai.ai_injector'
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
   self._entity_post_create_trace = radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         local ai_packs = radiant.entities.get_entity_data(entity, 'stonehearth:ai_packs')
         if ai_packs and ai_packs.packs then
            -- Discard the ai_handle because this injection is a permanent definition in the entity.
            -- i.e. We will never call ai_handle:destroy() to revoke the ai from entity_data.
            local ai_handle = self:inject_ai_packs(entity, ai_packs.packs)
         end
      end)
end

function AiService:inject_ai_packs(entity, packs)
   local ai = { ai_packs = packs}
   return self:inject_ai(entity, ai)
end

-- injecting entity may be null
function AiService:inject_ai(entity, ai) 
   return AiInjector(entity, ai)
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

function AiService:acquire_ai_lease(target, owner)
   if target and target:is_valid() and owner and owner:is_valid() then
      local o = {
         persistent = false,
      }
      return target:add_component('stonehearth:lease'):acquire(self.RESERVATION_LEASE_NAME, owner, o)
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

return AiService
