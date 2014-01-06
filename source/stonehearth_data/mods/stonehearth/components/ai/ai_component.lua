
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'
local log = radiant.log.create_logger('ai.component')

function AIComponent:__init(entity)
   self._entity = entity
   self._observers = {}
   self._actions = {}
   self._action_index = {}
   self._stack = {}
   self._execution_count = 0
   self._ai_system = radiant.mods.load('stonehearth').ai
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
   for name, action in pairs(self._actions) do
      if action.destroy then
         action:destroy(self._entity)
      end
   end
   self._observers = {}
   self._actions = {}
end

function AIComponent:add_action(uri, action)
   assert(not self._actions[uri])
   self._actions[uri] = action

   local does = action:get_activity()
   local action_index = self._action_index[does]
   if not action_index then
      action_index = {}
      self._action_index[does] = action_index
   end
   self._action_index[does][uri] = action
end

function AIComponent:remove_action(uri)
   local action = self._actions[uri]
   local does = action:get_activity()
   if action then
      self._actions[uri] = nil
      self._action_index[does][uri] = nil

      if action.destroy then
         action:destroy()
      end
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
      while not self._dead do
         self:execute('stonehearth:top')
      end
   end)
end

function AIComponent:_create_execution_frame(activity)
   local activity_name = select(1, unpack(activity))
  
   -- make sure this activity isn't in the stack
   for _, frame in ipairs(self._stack) do
      if frame:get_activity_name() == activity_name then
         self:abort('cannot run activity "%s".  already in the stack!', activity_name)
      end
   end

   -- create a new frame and return it   
   local actions = self._action_index[activity_name]

   log:spam('%s creating execution frame for %s', self._entity, activity_name)
   return ExecutionFrame(self, actions, activity)
end

function AIComponent:_terminate_thread()
   for i = #self._stack, 1, -1 do
      self._stack[i]:destroy(false)
   end
   self._stack = {}
   
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

function AIComponent:execute(...)
   -- get a set of valid actions for this excution frame 
   local activity = {...}
   local frame = self:_create_execution_frame(activity)
   
   self._execution_count = self._execution_count + 1
   table.insert(self._stack, frame)
   local stack_len = #self._stack

   local result = frame:execute(unpack(activity))   
   
   assert(#self._stack == stack_len)
   assert(self._stack[stack_len] == frame)
   frame:destroy()
   
   table.remove(self._stack)
   return unpack(result)
end

function AIComponent:suspend_thread()
   coroutine.yield(self._ai_system.SUSPEND_THREAD)
end

function AIComponent:resume_thread()
   self._ai_system:_resume_thread(self._co)
end

return AIComponent
