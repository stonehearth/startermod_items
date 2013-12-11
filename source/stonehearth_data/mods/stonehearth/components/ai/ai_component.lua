
local AIComponent = class()
local log = radiant.log.create_logger('ai.component')

function AIComponent:__init(entity)
   self._entity = entity
   self._observers = {}
   self._actions = {}
   self._action_stack = {}
   self._priority_table = {}
   self._ai_system = radiant.mods.load('stonehearth').ai
end

function AIComponent:extend(json)
   self._ai_system:start_ai(self._entity, json)
end

function AIComponent:destroy()
   self._ai_system:stop_ai(self._entity:get_id(), self)

   self._dead = true
   self:_clear_action_stack()

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
   self:set_action_priority(action, action.priority)
end

function AIComponent:remove_action(uri)
   local action = self._actions[uri]
   if action then
      self:remove_from_priority_table(action)
      self._actions[uri] = nil

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

function AIComponent:set_action_priority(action, priority)
   -- update the priority table
  
   local activity_name = action.does
   local priorities = self._priority_table[activity_name]
   if not priorities then
      self._priority_table[activity_name] = {}
      priorities = self._priority_table[activity_name]
   end
   priorities[action] = priority
   
   -- update the priority of the action in the action stack
   for i=1, #self._action_stack do
      local entry = self._action_stack[i]
      if entry.action == action then
         entry.priority = priority
      end
   end
end

function AIComponent:remove_from_priority_table(action)
   local activity_name = action.does
   local priorities = self._priority_table[activity_name]
   if priorities then
      priorities[action] = nil
   end
end

function AIComponent:check_action_stack()
   radiant.check.is_entity(self._entity)

   if not self._co or #self._action_stack == 0 then
      self:restart()
      return
   end

   -- walk the entire action stack looking for better things to do...
   local unwind_to_action = nil
   for i, entry in ipairs(self._action_stack) do
      local action, priority = self:_get_best_action(entry.activity, i-1)
      if action ~= entry.action and priority > entry.priority then
         log:debug('switching from %s to %s (priority:%d)', entry.action.name, action.name, priority)
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
      log:debug('behavior switching entity %s behavior:', tostring(self._entity))
      log:debug('   from: %s', self:_format_activity(self._current_activity))
      log:debug('   to:   %s', self:_format_activity(activity))
      self:restart(activity)
   end
   ]]
end

function AIComponent:restart()
   assert(not self._dead)

   radiant.check.is_entity(self._entity)
   self:_clear_action_stack()

   self._co = self._ai_system:_create_thread(self, function()
      while not self._dead do
         self:execute('stonehearth:top')
      end
   end)
end

function AIComponent:_get_best_action(activity, filter_depth)
   local activity_name = select(1, unpack(activity))
   local priorities = self._priority_table[activity_name]

   assert(priorities)

   -- return the new maximum
   local best_a, best_p, list_best_a

   for a, p in pairs(priorities) do
      log:debug('activity %s: %s has priority %d', activity_name, a.name, p)
      if not best_p or p >= best_p then
         -- get_best_action is called both to check the health of the current action
         -- stack as well as to choose new actions to put on the stack.  In the first
         -- case, we're simply checking to make sure that the action chosen at every
         -- level is the same as what is recorded in the stack, so finding the action
         -- in the stack is exactly what we want!  In the second case, we never, ever
         -- want to recur (infinite ai loop, anyone?), so we'll choose the next best
         -- action for ai:execute().  this is useful for inserting special actions into
         -- the graph and deferring to the other implementations as part of your
         -- implementation.
         if not self:_is_in_action_stack(a, filter_depth) then
            if not best_p or p > best_p then
                -- new best_p found, wipe out the old list of candidate actions
                list_best_a = {}
            end
            best_p = p

            local weight = a.weight or 1

            for i = 1, weight do
               table.insert(list_best_a, a)
            end
         end
      end
   end

   -- choose a random action amoung all the actions with the highest priority (they all tie)
   best_a = list_best_a[math.random(#list_best_a)]
   
   return best_a, best_p
end

--[[
function AIComponent:_get_best_activity()
   radiant.check.is_entity(self._entity)

   log:debug('computing best activity for %s.', tostring(self._entity))
   local bp, ba = -1, nil
   for name, action in pairs(self._actions) do
      local p, a = action:recommend_activity(self._entity)
      if p ~= nil then
         log:debug('  recommend activity %11d %s', p, self:_format_activity(a))
         if p > bp then
            bp, ba = p, a
         end
      end
   end
   assert(ba)
   log:debug('best activity for %s is %s.', tostring(self._entity), self:_format_activity(ba))
   return ba
end
]]

function AIComponent:_activities_equal(a, b)
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

function AIComponent:_format_activity(activity)
   if not activity then
      return 'none'
   end
   local msg = activity[1]
   for i = 2,#activity do
      msg = msg .. ", " .. tostring(activity[i])
   end
   return "{" .. msg .."}"
end

function AIComponent:_clear_action_stack()
   for i = #self._action_stack, 1, -1 do
      local action = self._action_stack[i].action
      if action.stop then
         action:stop(self, self._entity, false)
      end
   end
   self._action_stack = {}
   self._current_activity = nil
   if self._co then
      self._ai_system:_terminate_thread(self._co)
   end
end

function AIComponent:abort(reason)
   -- xxx: assert that we're running inthe context of the coroutine
   if reason == nil then reason = 'no reason given' end
   log:info('Aborting current action because: ' .. reason)
   
   self:_clear_action_stack()   
   -- all actions have had their stop method called on them.  yield
   -- KILL_THREAD to get the ai service to call restart() next time,
   -- which will start us over at stonehearth:top.   
   
   self._ai_system:_complete_thread_termination(self._co)   
end

function AIComponent:_is_in_action_stack(action, filter_depth)
   assert(filter_depth <= #self._action_stack, "we're looking too deep in _is_in_action_stack")
   if filter_depth > 0 then
      for i=1,filter_depth do
         local entry = self._action_stack[i]
         if entry.action == action then
            return true
         end
      end
   end
end

function AIComponent:execute(...)
   local activity = {...}
   local action, priority = self:_get_best_action(activity, #self._action_stack)

   local action_main = function()
      -- decoda_name = string.format("entity %d : %s action", self._entity:get_id(), tostring(action.name))
      --log:debug('coroutine starting action: %s for activity %s', action.name, self:_format_activity(activity))
      local result = { action:run(self, self._entity, select(2, unpack(activity))) }
      --log:debug('coroutine finished: %s', action.name)
      return result
   end

   local entry = {
      activity = activity,
      action = action,
      priority = priority,
      name = action.name and action.name or '- unnamed action -'
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

function AIComponent:wait_until(obj)
   coroutine.yield(obj)
end

function AIComponent:wait_for_path_finder(pf)
   local path
   pf:set_solved_cb(
      function(solution)
         path = solution
      end
   )
   self:wait_until(function()
      return path ~= nil
   end)
   return path
end

function AIComponent:suspend()
   coroutine.yield(self._ai_system.SUSPEND_THREAD)
end

function AIComponent:resume()
   self._ai_system:_resume_thread(self._co)
end

return AIComponent
