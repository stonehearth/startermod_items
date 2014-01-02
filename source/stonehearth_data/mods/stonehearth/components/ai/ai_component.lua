
local AIComponent = class()
local ExecutionFrame = require 'components.ai.execution_frame'
local log = radiant.log.create_logger('ai.component')

function AIComponent:__init(entity)
   self._entity = entity
   self._observers = {}
   self._actions = {}
   self._action_index = {}
   self._stack = {}
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
   self:_clear_stack()

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

   local action_index = self._action_index[action.does]
   if not action_index then
      action_index = {}
      self._action_index[action.does] = action_index
   end
   self._action_index[action.does][uri] = action
end

function AIComponent:remove_action(uri)
   local action = self._actions[uri]
   if action then
      self._actions[uri] = nil
      self._action_index[action.does][uri] = nil

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

function AIComponent:restart()
   assert(not self._dead)

   radiant.check.is_entity(self._entity)
   self:_clear_stack()

   self._co = self._ai_system:_create_thread(self, function()
      while not self._dead do
         self:execute('stonehearth:top')
      end
   end)
end

function AIComponent:_create_execution_frame(activity)
   local activity_name = select(1, unpack(activity))
   local action_index = self._action_index[activity_name]
   local actions = {}

   log:spam('%s creating execution frame for %s', self._entity, activity_name)
   for uri, action in pairs(action_index) do
      if self:_stack_contains(action) then
         log:spam('  ignoring action %s (already in stack)', action.name)
      else
         log:spam('  adding action %s', action.name)
         table.insert(actions, action)
      end
   end
   return ExecutionFrame(self, actions)
end

function AIComponent:_clear_stack()
   for i = #self._stack, 1, -1 do
      self._stack[i]:do_stop(false)
   end
   self._stack = {}
   if self._co then
      self._ai_system:_terminate_thread(self._co)
   end
end

function AIComponent:abort(reason)
   -- xxx: assert that we're running inthe context of the coroutine
   if reason == nil then reason = 'no reason given' end
   log:info('%s Aborting current action because: %s', self._entity, reason)
   
   self:_clear_stack()   
   -- all actions have had their stop method called on them.  yield
   -- KILL_THREAD to get the ai service to call restart() next time,
   -- which will start us over at stonehearth:top.   
   
   self._ai_system:_complete_thread_termination(self._co)   
end

function AIComponent:_stack_contains(action)
   for _, execution_frame in ipairs(self._stack) do
      if execution_frame:get_running_action() == action then
         return true
      end
   end
   return false
end

function AIComponent:execute(...)
   -- get a set of valid actions for this excution frame 
   local activity = {...}
   local execution_frame = self:_create_execution_frame(activity)
   
   table.insert(self._stack, frame)
   local stack_len = #self._stack

   local result = execution_frame:do_run(unpack(activity))   
   execution_frame:do_stop(true)

   assert(#self._stack == stack_len)
   assert(self._stack[stack_len] == execution_frame)
   table.pop(self._stack)
   return unpack(result)
end

function AIComponent:wait_until(obj)
   coroutine.yield(obj)
end

function AIComponent:suspend()
   coroutine.yield(self._ai_system.SUSPEND_THREAD)
end

function AIComponent:resume()
   self._ai_system:_resume_thread(self._co)
end

return AIComponent
