require 'unclasslib'
local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'

local Coroutine = class()
function Coroutine:__init(entity)
   check:is_entity(entity)

   self.entity = entity
   self:_reset()
end

function Coroutine:start(done, fn)
   log:debug('starting coroutine...')
   assert(not self.co)

   self.done = done and done or function() end
   self.co = coroutine.create(fn)
   self.pf = nil
end

function Coroutine:stop()
   if self.co then
      log:debug('stopping coroutine...')   
      self.done(false)
      self:_reset()
   end
end

function Coroutine:is_alive()
   log:debug('coroutine is %s', self.co and 'alive' or 'dead')
   return self.co
end

function Coroutine:_reset()
   log:debug('resetting coroutine...')
   self.co = nil
   self.done = function() end
end

function Coroutine:yield_next()
   log:debug('running coroutine...')
   if not self.co then
      return nil
   end

   assert(coroutine.status(self.co) == 'suspended')
   local success, result = coroutine.resume(self.co)
   if not success then
      check:report_thread_error(self.co, 'co-routine failed: ' .. tostring(result))
      self.done(false)
      self:_reset()
      return nil
   end

   if coroutine.status(self.co) == 'dead' then
      self.done(true)
      self:_reset()
   end
   return result -- xxx: what if the corutine yielded myltiple values?
end

function Coroutine:_do(msg, ...)
   log:debug('starting new "%s" action in coroutine.', msg)
   local handler = md:create_msg_handler(msg, ...)
   coroutine.yield(handler)
end

function Coroutine:follow_path(path)
   self:_do('radiant.actions.follow_path', self.entity, path)
end

function Coroutine:run_toward_point(dst)
   self:_do('radiant.actions.run_toward_point', self.entity, dst)
end

function Coroutine:perform_work(structure, block)
   local origin = om:get_world_grid_location(structure:get_entity())
   local facing = origin  + block
   log:info("------");
   log:info("structure is at %s working on block %s", tostring(origin), tostring(block))
   log:info("entity is at    %s", tostring(om:get_world_grid_location(self.entity)))
   om:turn_to_face(self.entity, facing)
   self:perform("work")
end

function Coroutine:perform(action, trigger_handler, args)
   if trigger_handler then check:is_callable(trigger_handler) end
   if args then check:is_table(args) end
   self:_do('radiant.actions.perform', action, trigger_handler, args)
end

function Coroutine:pickup(item)
   check:is_entity(item)

   local location = om:get_grid_location(item)
   local carry_block = om:get_component(self.entity, 'carry_block')

   -- TODO should this raise an exception if the entity isn't
   -- carrying anything?
 
   -- if we're not close enough to the location, we should
   -- travel there!
   if not carry_block:is_carrying() then
      local standing = om:get_grid_location(self.entity)
      if location:is_adjacent_to(standing) then
         log:info('picking up item at %s', tostring(location))
         om:remove_from_terrain(item)
         carry_block:set_carrying(item)
         om:move_entity_to(item, RadiantIPoint3(0, 0, 0))
         om:turn_to_face(self.entity, location)
         self:perform('carry_light_pickup')
      end
   end
end

function Coroutine:drop_carrying(location)
   -- TODO should this raise an exception if the entity isn't
   -- carrying anything?

   -- if we're not close enough to the location, we should
   -- travel there!
   local carry_block = om:get_component(self.entity, 'carry_block')
   if carry_block:is_carrying() then
      local item = carry_block:get_carrying()
      om:turn_to_face(self.entity, location)
      self:perform('carry_light_putdown')
      carry_block:set_carrying(nil)
      om:place_on_terrain(item, location)
   end
end

function Coroutine:construct(structure, block)
   local carry_block = om:get_component(self.entity, 'carry_block')
   if structure:get_class_name() == 'Fixture' then
      structure:set_item(om:create_entity(structure:get_kind()))
   else
      if carry_block:is_carrying() then
         self:perform_work(structure, block);
         
         -- xxx: move everything below to the worker scheduler done callback
         structure:construct_block(block)
         local carrying = carry_block:get_carrying()
         local item = om:get_component(carrying, 'item')
         local stacks = item:get_stacks()
         if stacks > 1 then
            item:set_stacks(stacks - 1)         
         else
            carry_block:set_carrying(nil)
            om:destroy_entity(carrying)
         end
      end
   end
end

function Coroutine:teardown(structure, block)
   if not om:can_teardown_structure(self.entity, structure) then
      self:stop() -- is calling stop inside a coroutine actually legit?
      return
   end
   
   self:perform_work(structure, block);
   
   -- xxx: move everything below to the worker scheduler done callback
   structure:construct_block(block)
   om:teardown(self.entity, structure)
end

function Coroutine:travel_to_item(item)
   check:is_entity(item)
   local path = nil
   self:_do('radiant.actions.find_path_to_item', self.entity, item, function(p)
      path = p
   end)
   if path then
      self:follow_path(path);
   end
end

return Coroutine
