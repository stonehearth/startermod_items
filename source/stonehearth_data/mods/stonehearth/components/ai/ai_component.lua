
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'
local log = radiant.log.create_logger('ai.component')
local ExecutionUnitV1 = require 'components.ai.execution_unit_v1'
local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'

local URI_TO_ACTIVITY = {}

function AIComponent:__init(entity)
   self._entity = entity
   self._observers = {}
   self._action_index = {}
   self._execution_count = 0
   self._ai_system = stonehearth.ai
end

function AIComponent:extend(json)
   self._ai_system:start_ai(self._entity, json)
end

function AIComponent:get_entity()
   return self._entity
end

function AIComponent:destroy()
   self._ai_system:stop_ai(self._entity:get_id(), self)

   self._dead = true
   self:_terminate_thread()

   for obs, _ in pairs(self._observers) do
      if obs.destroy then
         obs:destroy(self._entity)
      end
   end
   self._observers = {}
   self._action_index = {}
end

function AIComponent:add_action(uri, action_ctor, injecting_entity)
   local does = action_ctor.does
   assert(does)
   URI_TO_ACTIVITY[uri] = does
   local action_index = self._action_index[does]
   if not action_index then
      action_index = {}
      self._action_index[does] = action_index
   end
   assert(not self._action_index[does][uri])
   self._action_index[does][uri] = {
      action_ctor = action_ctor,
      injecting_entity = injecting_entity
   }
end

function AIComponent:remove_action(uri)
   local does = URI_TO_ACTIVITY[uri]
   if does then
      self._action_index[does][uri] = nil
      assert(false) -- need to find the lowest execution_frame that contains this action and restart it
   end
end

function AIComponent:add_observer(uri, observer)
   assert(not self._observers[uri])
   self._observers[uri] = observer
end

function AIComponent:remove_observer(observer)
   if observer then
      if observer.destroy_observer then
         observer:destroy_observer()
      end
      self._observer[uri] = nil
   end
end

-- shouldn't need this...  threads should restart themselves... but
-- what if we die while trying to restart?  ug!
function AIComponent:restart_if_terminated()
   if not self._co then
      self:restart()
   end
end

function AIComponent:restart()
   assert(not self._dead)

   radiant.check.is_entity(self._entity)
   self:_terminate_thread()
   
   self._co = self._ai_system:_create_thread(self, function()
      self._execution_frame = self:spawn('stonehearth:top')
      while not self._dead do
         self._execution_frame:loop()
      end
   end)
end

function AIComponent:spawn(...)
   local activity = { ... }
   local activity_name = select(1, unpack(activity))
  
   -- create a new frame and return it   
   local execution_units = {}
   for uri, entry in pairs(self._action_index[activity_name]) do
      local action, execution_unit_ctor
      local ctor = entry.action_ctor
      local injecting_entity = entry.injecting_entity
      if not ctor.version or ctor.version == 1 then
         execution_unit_ctor = ExecutionUnitV1
      else
         execution_unit_ctor = ExecutionUnitV2
      end
      local unit = execution_unit_ctor(self, self._entity, injecting_entity)
      action = ctor(unit, self._entity, injecting_entity)
      unit:set_action(action)
      execution_units[uri] = unit
   end

   log:spam('%s creating execution frame for %s', self._entity, activity_name)
   return ExecutionFrame(self, execution_units, activity)
end

function AIComponent:_terminate_thread()
   if self._execution_frame then
      self._execution_frame:abort('terminating thread')
   end
   
   if self._co then
      local co = self._co
      self._co = nil
      
      -- If we're calling _terminate_thread() from the co-routine itself,
      -- this function MAY NOT RETURN!
      self._ai_system:_terminate_thread(co)      
   end
end

function AIComponent:abort(reason)
   -- xxx: assert that we're running inthe context of the coroutine
   if reason == nil then reason = 'no reason given' end
   log:info('%s Aborting current action because: %s', self._entity, reason)  
   self:_terminate_thread()
end

function AIComponent:suspend_thread()
   coroutine.yield(self._ai_system.SUSPEND_THREAD)
end

function AIComponent:resume_thread()
   self._ai_system:_resume_thread(self._co)
end

return AIComponent
