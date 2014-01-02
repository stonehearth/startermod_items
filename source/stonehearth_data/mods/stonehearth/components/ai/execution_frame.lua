
local ExecutionFrame = class()
local log = radiant.log.create_logger('ai.component')

function ExecutionFrame:__init(ai, actions)
   self._ai = ai
   self._actions = actions
end

function ExecutionFrame:_execute(activity_name, ...)
   self._activity_name = activity_name
   self._args = { ... }
   local action, priority = self:_choose_action()
   self:_run()
   return result
end

function ExecutionFrame:set_action_priority(action, priority)
   local abort = false
   if action == self._running_action then
      -- if the current action is trying to change its priority, we need to check the
      -- current frame if that might cause some other action to be run.  yes, this is
      -- a weird thing for an action to do, but it's easy to be robust here.
      if priority < action.priority then
         -- going lower doesn't mean we're the lowest!  check now...
         local best_action = self._choose_action()
         abort = action ~= best_action
      end
   else
      -- if another action is elevating it's priority above the current action
      -- abort.
      abort = priority > self._running_action.priority
   end

   action.priority = priority
   if abort then
      self._ai:abort(string.format('action %s elevated priority above running action %s (%d > %d)',
                                   action.name, priority, self._running_action.name,
                                   self._running_action.priority))
   end
end

function ExecutionFrame:execute(...)
   self._ai:execute(...)
end

function ExecutionFrame:abort(reason)
   self._ai:abort(reason)
end

function ExecutionFrame:wait_until(obj)
   self._ai:wait_until(obj)
end

function ExecutionFrame:suspend()
   self._ai:suspend()
end

function ExecutionFrame:resume()
   self._ai:resume()
end

function ExecutionFrame:wait_for_path_finder(pf)
   local path
   pf:set_solved_cb(
      function(solution)
         path = solution
      end
   )
   log:debug('%s blocking until pathfinder finishes', self._entity)
   self:wait_until(function()
      if path ~= nil then
         log:debug('%s pathfinder completed!  resuming action', self._entity)
         return true
      end
      if pf:is_idle() then
         log:debug('%s pathfinder went idle.  aborting!', self._entity)
         self:abort('pathfinder unexpectedly went idle while finding path')
      end
      log:debug('%s waiting for pathfinder: %s', self._entity, pf:describe_progress())
      return false
   end)
   return path
end


function ExecutionFrame:get_running_action()
   return self._running_action
end

function ExecutionFrame:do_run(action)
   self._running_action = action
   log:debug('%s coroutine starting action: %s for activity %s(%s)', self._entity, tostring(action.name), self._activity_name, self:_format_args(self.args))
   local result = { action:run(self, self._entity, unpack(self._args)) }
   log:debug('%s coroutine finished: %s', self._entity, tostring(action.name))   
end

function ExecutionFrame:do_stop(success)
   if self._running_action and self._running_action.stop then
      self._running_action:stop(success)
   end
   self._running_action = nil
end

function ExecutionFrame:_format_args(args)
   local msg = ''
   if args then
      for _, arg in ipairs(args) do
         if #msg > 0 then
            msg = msg .. ', '
         end
         msg = msg .. tostring(arg)
      end
   end
   return msg
end

function ExecutionFrame:_choose_action()
   local entity = self._ai:get_entity()
   local best_priority = 0
   local best_actions = {}
   
   log:spam('%s looking for best action for %s', _entity, self._activity_name)
   for _, action in ipairs(self._actions) do      
      log:spam('  action %s has priority %d', tostring(action.name), action.priority)

      if action.priority >= best_priority then
         if action.priority > best_priority then
            -- new best_priority found, wipe out the old list of candidate actions
            best_actions = {}
         end
         best_priority = action.priority
         local weight = a.weight or 1
         for i = 1, weight do
            table.insert(best_actions, a)
         end
      end
   end

   -- choose a random action amoung all the actions with the highest priority (they all tie)
   best_action = best_actions[math.random(#best_actions)]
   log:spam('%s  best action for %s is %s (priority: %d)', self._entity, activity_name, tostring(best_action.name), best_priority)
   
   return best_action, best_priority
end
