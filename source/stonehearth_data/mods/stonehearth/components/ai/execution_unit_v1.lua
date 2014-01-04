
local ExecutionUnitV1 = class()
local log = radiant.log.create_logger('ai.component')

local IDLE = 'IDLE'
local RUNNING = 'RUNNING'

function ExecutionUnitV1:__init(ai_component, entity, injecting_entity)
   self._entity = entity
   self._ai_component = ai_component
   self._state = IDLE
end

-- AiService facing functions...
function ExecutionUnitV1:set_action(action)
   if self._action then
      -- reached if set_action_priority was called in the action
      -- construtor.  make sure there's no funny business going on.
      assert(action == self._action)
   end
   self._action = action
end

-- ExecutionFrame facing functions...
function ExecutionUnitV1:get_activity()
   return self._action.does
end

function ExecutionUnitV1:get_name()
   return self._action.name
end

function ExecutionUnitV1:get_frame()
   return self._frame
end

function ExecutionUnitV1:get_priority()
   return self._action.priority
end

function ExecutionUnitV1:get_weight()
   return self._action.weight or 1
end

function ExecutionUnitV1:is_runnable(frame)
   assert(self._frame == frame)
   return self._action.priority > 0   
end

function ExecutionUnitV1:join_execution_frame(frame, args)
   assert(not self._frame)
   self._frame = frame
   self._args = args
end

function ExecutionUnitV1:leave_execution_frame(frame)
   assert(self._frame == frame)
   self._frame = nil
   self:stop_frame()
end

function ExecutionUnitV1:stop_frame()
   if self._state == RUNNING then
      if self._action.stop then
         self._action:stop()
      end
      self._state = IDLE
   end
end

function ExecutionUnitV1:execute_frame(frame)
   assert(self._frame == frame)
   log:debug('%s coroutine starting action: %s for activity %s(%s)',
             self._entity, self:get_name(), self._frame:get_activity_name(), self._frame:format_args(self._args))
   
   self._state = RUNNING
   local result = { self._action:run(self, self._entity, unpack(self._args)) }
   
   log:debug('%s coroutine finished: %s', self._entity, tostring(self:get_name()))
   return result
end

-- Action facing functions...
function ExecutionUnitV1:set_action_priority(action, priority)
   self:set_action(action)
   if priority ~= self._action.priority then
      self._action.priority = priority
      -- notify the frame if we're currently in one.  we might not be! legacy
      -- actions act crazily sometimes...
      if self._frame then
         self._frame:on_action_priority_changed(self, priority)
      end
   end
end

function ExecutionUnitV1:execute(...)
   return self._ai_component:execute(...)
end

function ExecutionUnitV1:wait_until(obj)
   coroutine.yield(obj)
end

function ExecutionUnitV1:suspend()
   self._ai_component:suspend_thread()
end

function ExecutionUnitV1:resume()
   self._ai_component:resume_thread()
end

function ExecutionUnitV1:abort(reason)
   self._ai_component:abort(reason)
end

function ExecutionUnitV1:wait_for_path_finder(pf)
   local path = pf:get_solution()
   if not path then
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
   end
   return path
end

return ExecutionUnitV1
