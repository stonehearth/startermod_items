local BehaviorManager = class()

function BehaviorManager:__init(entity)
   assert(entity)
   self._entity = entity
   self._observers = {}
   self._actions = {}
   self._action_stack = {}
   self._priority_table = {}
end

function BehaviorManager:destroy()
   self._dead = true
   self:_clear_action_stack()

   if self._debug_hook then
      self._debug_hook:notify_current_thread(nil)
   end

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

function BehaviorManager:set_debug_hook(hookfn)
   self._debug_hook = hookfn
   self._debug_hook:notify_current_thread(self._co)
end

function BehaviorManager:get_debug_hook()
   return self._debug_hook
end

function BehaviorManager:add_action(uri, action)
   assert(not self._actions[uri])
   self._actions[uri] = action

   self:set_action_priority(action, action.priority)
end

function BehaviorManager:remove_action(uri)
   local action = self._actions[uri]
   if action then
      if action.destroy_action then
         action:destroy_action()
      end
      self._actions[uri] = nil
   end
end

function BehaviorManager:add_observer(uri, observer)
   assert(not self._observers[uri])
   self._observers[uri] = observer
end

function BehaviorManager:remove_observer(uri)
   local observer = self._observer[uri]
   if observer then
      if observer.destroy_observer then
         observer:destroy_observer()
      end
      self._observer[uri] = nil
   end
end

function BehaviorManager:set_action_priority(action, priority)
   -- update the priority table
   local activity_name = action.does
   local priorities = self._priority_table[activity_name]
   if not priorities then
      self._priority_table[activity_name] = {}
      priorities = self._priority_table[activity_name]
   end
   priorities[action] = priority
end

function BehaviorManager:remove_from_priority_table(action)
   local activity_name = action.does
   local priorities = self._priority_table[activity_name]
   if priorities then
      priorities[action] = nil
   end
end

function BehaviorManager:check_action_stack()
   radiant.check.is_entity(self._entity)

   if not self._co or #self._action_stack == 0 then
      self:restart()
      return
   end

   -- walk the entire action stack looking for better things to do...
   local unwind_to_action = nil
   for i, entry in ipairs(self._action_stack) do
      local action, priority = self:_get_best_action(entry.activity)
      if action ~= entry.action then
         radiant.log.warning('switching from %s to %s (priority:%d)', entry.action.name, action.name, priority)
         unwind_to_action = i
         break
      end
   end
   if unwind_to_action then
      self:restart()
   end
   --[[
   self._action_stack
   local activity = self:_get_best_activity()
   if not self._current_activity or not self:_activities_equal(activity, self._current_activity) then
      radiant.log.debug('behavior switching entity %s behavior:', tostring(self._entity))
      radiant.log.debug('   from: %s', self:_format_activity(self._current_activity))
      radiant.log.debug('   to:   %s', self:_format_activity(activity))
      self:restart(activity)
   end
   ]]
end

function BehaviorManager:restart()
   assert(not self._dead)

   radiant.check.is_entity(self._entity)
   self:_clear_action_stack()


   self._co = radiant.ai._create_thread(self, function()
      while not self._dead do
         self:execute('stonehearth.activities.top')
      end
   end)

   if self._debug_hook then
      self._debug_hook:notify_current_thread(self._co)
   end
end

function BehaviorManager:_get_best_action(activity)
   local activity_name = select(1, unpack(activity))
   local priorities = self._priority_table[activity_name]

   assert(priorities)

   -- return the new maximum
   local best_a, best_p
   for a, p in pairs(priorities) do
      if a.get_priority then
         p = a:get_priority(unpack(activity))
         priorities[a] = p
      end
      --radiant.log.info('activity %s: %s has priority %d', activity_name, a.name, p)
      if not best_p or p > best_p then
         best_a, best_p = a, p
      end
   end
   return best_a, best_p
end

--[[
function BehaviorManager:_get_best_activity()
   radiant.check.is_entity(self._entity)
   local debug_info = { priorities = {} }
   k
   radiant.log.debug('computing best activity for %s.', tostring(self._entity))
   local bp, ba = -1, nil
   for name, action in pairs(self._actions) do
      local p, a = action:recommend_activity(self._entity)
      if p ~= nil then
         radiant.log.debug('  recommend activity %11d %s', p, self:_format_activity(a))
         if self._debug_hook then
            debug_info.priorities[name] = {  priority = p, activity = self:_format_debug_activity(a) }
         end
         if p > bp then
            bp, ba = p, a
         end
      end
   end
   if self._debug_hook then
      self._debug_hook:set_ai_priorities(debug_info)
   end
   assert(ba)
   radiant.log.debug('best activity for %s is %s.', tostring(self._entity), self:_format_activity(ba))
   return ba
end
]]

function BehaviorManager:_activities_equal(a, b)
   if not a or not b then
      return false
   end

   assert(type(a) == 'table')
   assert(type(b) == 'table')
   if #a ~= #b then
      return false
   end
   for i = 1, #a do
      local a_i = a[i]
      local b_i = b[i]

      if a_i == b_i then
         return true
      end

      local a_type = type(a_i)
      local b_type = type(b_i)
      if a_type ~= b_type then
         return false
      end

      if a_type == 'userdata' then
         local a_key = a_i.get_id and a_i:get_id() and a_i:__towatch()
         local b_key = b_i.get_id and b_i:get_id() and b_i:__towatch()
         return a_key == b_key
      end

      -- table?  etc...
      return false
   end
   return true
end

function BehaviorManager:_format_activity(activity)
   if not activity then
      return 'none'
   end
   local msg = activity[1]
   for i = 2,#activity do
      msg = msg .. ", " .. tostring(activity[i])
   end
   return "{" .. msg .."}"
end

function BehaviorManager:_format_debug_activity(activity)
   local result = {}
   for i = 1,#activity do
      local entry = activity[i]
      local et = type(entry)
      if et == 'number' or et == 'string' then
         result[i] = entry
      else
         result[i] = tostring(entry)
      end
   end
   return result
end

function BehaviorManager:_clear_action_stack()
   for i = #self._action_stack, 1, -1 do
      local action = self._action_stack[i].action
      if action.stop then
         action:stop(self, self._entity, false)
      end
   end
   self._action_stack = {}
   self._current_activity = nil
   if self._co then
      radiant.ai._terminate_thread(self._co)
   end
end

function BehaviorManager:abort(reason)
   -- xxx: assert that we're running inthe context of the coroutine
   assert(false)
end

function BehaviorManager:execute(...)
   local activity = {...}
   local action = self:_get_best_action(activity)

   local action_main = function()
      -- decoda_name = string.format("entity %d : %s action", self._entity:get_id(), tostring(action.name))
      --radiant.log.debug('coroutine starting action: %s for activity %s', action.name, self:_format_activity(activity))
      local result = { action:run(self, self._entity, select(2, unpack(activity))) }
      --radiant.log.debug('coroutine finished: %s', action.name)
      return result
   end

   local entry = {
      activity = activity,
      action = action
   }
   table.insert(self._action_stack, entry)
   local len = #self._action_stack

   -- just call it and hope for the best.  manually unwind at yields...
   local result = action_main()

   --[[
   -- this is a loser. trying to yield in an pcall in a coroutine just doesn't work.
   local traceback
   local success, error = xpcall(action_main, function() traceback = debug.traceback() end)
   if not success then
      print(debug.traceback(co))
      error(result)
   end
   ]]

   --[[
   -- this is a loser.  trying to unwind the coroutine stack hits the dreaded "yield
   -- across c boundary", and coco is super expensive (fibers on windows)
   local success, result
   while coroutine.status(entry.co) ~= 'dead' do
      success, result = coroutine.resume(entry.co)
      print('--', success, result)
      if not success then
         print(debug.traceback(co))
         error(result)
      end
      -- yield once.  this will end up unwinding the whole damn stack,
      -- all the way out to the outer action (returning back to the
      -- ai manager)
      coroutine.yield(result)
   end
   ]]
   if action.stop then
      action:stop(self)
   end

   assert(#self._action_stack == len)
   assert(self._action_stack[len].action == action)
   self._action_stack[len] = nil
   return unpack(result)

--[[
   local result = { action:run(self, self._entity, select(2, unpack(activity))) }
   if self._dead then
      assert(#self._action_stack == 0)
      return
   end

   if action.stop then
      action:stop(self, self._entity, true)
   end

   assert(#self._action_stack == len)
   assert(self._action_stack[len] == action)
   self._action_stack[len] = nil
   return unpack(result)
]]

end

function BehaviorManager:wait_until(obj)
   coroutine.yield(obj)
end

function BehaviorManager:suspend()
   coroutine.yield(radiant.ai.SUSPEND_THREAD)
end

function BehaviorManager:resume()
   radiant.ai._resume_thread(self._co)
end

return BehaviorManager
