local Point3 = _radiant.csg.Point3

local StateMachine = class()

function StateMachine.add_nop_state_transition(cls, msg, state)
   local method = '_' .. msg .. '_from_' .. state
   assert(not cls[method])
   cls[method] = function() end
end

function StateMachine:__init(instance, log)
   self._log = log
   self._instance = instance
end

function StateMachine:in_state(...)
   local states = { ... }
   for _, state in ipairs(states) do
      if self._state == state then
         return true
      end
   end
   return false
end

function StateMachine:get_state()
   return self._state
end

function StateMachine:set_state(state)
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   self._state = state
   
   if self._state == self._waiting_until then
      self._calling_thread:resume('transitioned to ' .. self._state)
   end
end

function StateMachine:wait_until(state)
   self._calling_thread = stonehearth.threads:get_current_thread()   
   if self._state ~= state then
      self._waiting_until = state
      while self._state ~= state do
         self._calling_thread:suspend('waiting for ' .. state)
      end
      self._waiting_until = nil
      self._calling_thread = nil
   end
end

function StateMachine:dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local dispatch_msg_fn = self._instance[method]

   if not dispatch_msg_fn then
      -- try the 'anystate' fallback
      local method = '_' .. msg .. '_from_anystate'
      dispatch_msg_fn = self._instance[method]
   end
   self._log:spam('dispatching %s', method)
   assert(dispatch_msg_fn, string.format('unknown state transition in execution frame "%s"', method))
   dispatch_msg_fn(self._instance, ...)
end

return StateMachine
 