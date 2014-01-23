local Point3 = _radiant.csg.Point3

local StateMachine = class()

function StateMachine.add_nop_state_transition(cls, msg, state)
   local method = '_' .. msg .. '_from_' .. state
   assert(not cls[method])
   cls[method] = function() end
end

function StateMachine:__init(instance, thread, log)
   self._log = log
   self._instance = instance
   self._thread = thread
   self._thread:set_msg_handler(function (...) self:_dispatch_msg(...) end)
end


function StateMachine:in_state(...)
   for _, state in ipairs({...}) do
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

-- external interface from another thread.  blocking.
function StateMachine:protect_blocking_call(blocking_call_fn)
   assert(not self._calling_thread)

   self._calling_thread = stonehearth.threads:get_current_thread()   
   assert(self._calling_thread ~= self._thread)
   local result = { blocking_call_fn(self._calling_thread) }
   self._calling_thread = nil

   return unpack(result)
end

function StateMachine:wait_until(state)
   assert(self._calling_thread)
   assert(self._calling_thread:is_running())
   
   if self._state ~= state then
      self._waiting_until = state
      while self._state ~= state do
         self._calling_thread:suspend('waiting for ' .. state)
      end
      self._waiting_until = nil
   end
end

function StateMachine:_dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local dispatch_msg_fn = self._instance[method]

   if not dispatch_msg_fn then
      -- try the 'anystate' fallback
      local method = '_' .. msg .. '_from_anystate'
      dispatch_msg_fn = self._instance[method]
   end
   assert(dispatch_msg_fn, string.format('unknown state transition in execution frame "%s"', method))
   self._log:spam('dispatching %s', method)
   dispatch_msg_fn(self._instance, ...)
end


function StateMachine:loop_until(terminal_state)
   assert(self._thread:is_running())
   while self._state ~= terminal_state do
      self._thread:suspend('state machine is idle')
   end
end

return StateMachine
 