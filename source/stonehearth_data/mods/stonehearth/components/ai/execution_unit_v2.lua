local ExecutionUnitV2 = class()
local log = radiant.log.create_logger('ai.component')

local IDLE = 'IDLE'
local RUNNING = 'RUNNING'

function ExecutionUnitV2:__init(ai_component, entity, injecting_entity)
   self._entity = entity
   self._ai_component = ai_component
end

-- AiService facing functions...
function ExecutionUnitV2:set_action(action)
   self._action = action
end

-- ExecutionFrame facing functions...
function ExecutionUnitV2:get_activity()
   return self._action.does
end

function ExecutionUnitV2:get_name()
   return self._action.name
end

function ExecutionUnitV2:get_frame()
   return self._frame
end

function ExecutionUnitV2:get_priority()
   return self._action.priority
end

function ExecutionUnitV2:get_weight()
   return self._action.weight or 1
end

function ExecutionUnitV2:is_runnable(frame)
   assert(self._frame == frame)
   return self._action.priority > 0   
end

function ExecutionUnitV2:join_execution_frame(frame, args)
   assert(not self._frame)
   self._frame = frame
   self._args = args
end

function ExecutionUnitV2:leave_execution_frame(frame)
   assert(self._frame == frame)
   self._frame = nil
   self:stop_frame()
end

function ExecutionUnitV2:stop_frame()
   if self._state == RUNNING then
      if self._action.stop then
         self._action:stop()
      end
      self._state = IDLE
   end
end

function ExecutionUnitV2:execute_frame(frame)
   assert(self._frame == frame)
   log:debug('%s coroutine starting action: %s for activity %s(%s)',
             self._entity, self:get_name(), self._frame:get_activity_name(), self._frame:format_args(self._args))
   
   self._state = RUNNING
   local result = { self._action:run(self, self._entity, unpack(self._args)) }
   
   log:debug('%s coroutine finished: %s', self._entity, tostring(self:get_name()))
   return result
end

-- Action facing functions...
function ExecutionUnitV2:set_action_priority(action, priority)
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

function ExecutionUnitV2:execute(...)
   return self._ai_component:execute(...)
end

function ExecutionUnitV2:suspend()
   self._ai_component:suspend_thread()
end

function ExecutionUnitV2:resume()
   self._ai_component:resume_thread()
end

function ExecutionUnitV2:abort(reason)
   self._ai_component:abort(reason)
end

return ExecutionUnitV2
