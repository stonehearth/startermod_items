local ExecutionFrame = require 'components.ai.execution_frame'
local ExecutionUnitV2 = class()

local THINKING = 'thinking'
local READY = 'ready'
local STARTED = 'started'
local RUNNING = 'running'
local FINISHED = 'finished'
local HALTED = 'halted'
local STOPPED = 'stopped'
local DEAD = 'dead'
local ABORTED = 'aborted'
local placeholders = require 'services.ai.placeholders'

function ExecutionUnitV2:__init(parent_thread, entity, injecting_entity, action, args, action_index)
   assert(action.name)
   assert(action.does)
   assert(action.args)
   assert(action.priority)

   self._args = args
   self._action = action
   self._action_index = action_index
   self._think_output_types = self._action.think_output or self._action.args
   self._entity = entity
   self._execute_frame = nil
   self._thinking = false
   self._id = stonehearth.ai:get_next_object_id()
     
   self._ai_interface = { ___execution_unit = self }   
   local chain_function = function (fn)
      self._ai_interface[fn] = function(i, ...) return self['__'..fn](self, ...) end
   end
   chain_function('set_think_output')
   chain_function('revoke_think_output')
   chain_function('spawn')
   chain_function('execute')   
   chain_function('suspend')
   chain_function('resume')
   chain_function('abort')
   chain_function('get_log')   

   self._parent_thread = parent_thread
   self._thread = parent_thread:create_thread()
                               :set_debug_name('u:%d', self._id)
                               :set_msg_handler(function (...) self:_dispatch_msg(...) end)
                               :set_thread_main(function (...) self:_thread_main(...) end)

   self._log = radiant.log.create_logger('ai.exec_unit')
   local prefix = string.format('%s (%s)', self._thread:get_debug_route(), self:get_name())
   self._log:set_prefix(prefix)
   self._log:debug('creating execution unit')
   
   if self:_verify_arguments(args, self._action.args) then
      self:_set_state(STOPPED)
   else
      assert(false, 'put ourselves in the dead state')
   end
   
   self._thread:start()
end

function ExecutionUnitV2:get_action_interface()
   return self._ai_interface
end

function ExecutionUnitV2:get_action()
   return self._action
end

-- ExecutionFrame facing functions...
function ExecutionUnitV2:get_name()
   return self._action.name
end

function ExecutionUnitV2:get_priority()
   return self._action.priority
end

function ExecutionUnitV2:get_weight()
   return self._action.weight or 1
end

function ExecutionUnitV2:is_runnable()
   return self._action.priority > 0 and self._state == READY
end

function ExecutionUnitV2:is_active()
   return self:is_runnable() or self._state == STARTED or self._state == RUNNING or self._state == FINISHED
end

function ExecutionUnitV2:get_current_entity_state()
   assert(self._current_entity_state)
   return self._current_entity_state
end

function ExecutionUnitV2:_get_action_debug_info()
   if self._action.get_debug_info then
      return self._action:get_debug_info(self, self._entity)
   end
   return {
      name = self._action.name,
      does = self._action.does,
      priority = self._action.priority,
      args = stonehearth.ai:format_args(self._args),
   }
end

function ExecutionUnitV2:get_debug_info()
   local info = {
      id = self._id,
      state = self._state,
      action = self:_get_action_debug_info(),
      think_output = stonehearth.ai:format_args(self._think_output),
      execution_frames = {}
   }
   if self._execute_frame then
      table.insert(info.execution_frames, self._execute_frame:get_debug_info())
   end
   return info
end

function ExecutionUnitV2:send_msg(...)
   self._thread:send_msg(...)
end

function ExecutionUnitV2:_thread_main()
   while self._state ~= DEAD do
      self._thread:suspend('waiting for execution unit to die')
   end
end

function ExecutionUnitV2:_dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local dispatch_msg_fn = self[method]
   assert(dispatch_msg_fn, string.format('unknown state transition in execution frame "%s"', method))   
   dispatch_msg_fn(self, ...)
end

function ExecutionUnitV2:_call_action(method)
   local call_action_fn= self._action[method]
   if call_action_fn then
      self._calling_method = method
      call_action_fn(self._action, self._ai_interface, self._entity, self._args)
      self._calling_method = nil
   else
      self._log:detail('action does not implement %s.', method)
   end
   return call_action_fn ~= nil
end

function ExecutionUnitV2:_start_thinking_from_stopped(entity_state)
   assert(not self._thinking)

   self._thinking = true
   self._current_entity_state = entity_state
   self._ai_interface.CURRENT = entity_state

   self:_set_state(THINKING)
   if not self:_call_action('start_thinking') then
      self:send_msg('set_think_output')
   end
end

function ExecutionUnitV2:_set_think_output_from_thinking(think_output)
   if not think_output then
      think_output = self._args
   end
   self:_verify_arguments(think_output, self._think_output_types)

   self._parent_thread:send_msg('unit_ready', self, think_output)
   self:_set_state(READY)
end

function ExecutionUnitV2:_revoke_think_output_from_thinking(entity_state)
   self._parent_thread:send_msg('unit_not_ready', self)
   self:_set_state(THINKING)
end


function ExecutionUnitV2:_stop_thinking_from_thinking()
   assert(self._thinking)
   self:_stop_thinking()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_start_from_ready()
   assert(self._thinking)

   self:_stop_thinking()
   self:_call_action('start') 
   self:_set_state(STARTED)
end

function ExecutionUnitV2:_run_from_started()
   assert(not self._thinking)

   self:_set_state(RUNNING)
   self:_call_action('run')
   self:_set_state(FINISHED)
   self:send_msg('stop')
end

function ExecutionUnitV2:_stop_from_finished()
   assert(not self._thinking)

   self:_call_action('stop')
   self._set_state(FINISHED)
end

function ExecutionUnitV2:_stop_thinking()
   self._log:debug('stop_thinking (state:%s)', tostring(self._state))
   assert(self._thinking)

   self._current_entity_state = nil
   self._ai_interface.CURRENT = nil
   self._thinking = false

   self:_call_action('stop_thinking')
end

-- the interface facing actions (i.e. the 'ai' interface)

function ExecutionUnitV2:__get_log()
   return self._log
end

function ExecutionUnitV2:__execute(name, args)
   self._log:debug('__execute %s %s called', name, stonehearth.ai:format_args(args))
   assert(self._thread:is_running())
   assert(not self._execute_frame)

   self._execute_frame = ExecutionFrame(self._thread, self._entity, name, args, self._action_index)

   self._execute_frame:run()
   self._execute_frame:stop()
   self._execute_frame:destroy()
   self._execute_frame = nil
end 

function ExecutionUnitV2:__set_think_output(args)
   self._log:debug('__set_think_output %s called', stonehearth.ai:format_args(args))
   self:send_msg('set_think_output', args)
end

function ExecutionUnitV2:__revoke_think_output()
   self._log:debug('__revoke_think_output %s called')
   self:send_msg('revoke_think_output')
end

function ExecutionUnitV2:__spawn(name, args)   
   self._log:debug('__spawn %s %s called', name, stonehearth.ai:format_args(args))
   assert(self._thread:is_running())
  
   return ExecutionFrame(self._thread, self._entity, name, args, self._action_index)
end

function ExecutionUnitV2:__suspend(format, ...)
   assert(self._thread:is_running())
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__suspend called (reason: %s)', reason)
   self._thread:suspend(reason)
end

function ExecutionUnitV2:__resume(format, ...)
   assert(not self._thread:is_running())
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__resume called (reason: %s)', reason)
   self._thread:resume(reason)
end

function ExecutionUnitV2:__abort(reason, ...)
   local reason = string.format(reason, ...)
   self._log:debug('__abort %s called (state: %s)', tostring(reason), self._state)
   if self._thread:is_running() then
      assert(false, 'cleanup here...')
      self._thread:terminate()
   else
      self._thread:set_msg('abort', reason)
   end
end

--[[
function ExecutionUnitV2:destroy()
   self._log:spam('destroy')
   if self._execute_frame then
      self._execute_frame:destroy()
      self._execute_frame = nil
   end
   
   self._log:spam('destroying action')
   self:stop_thinking()   
   if self._state == IDLE or self._state == READY or self._state == DEAD then
      -- nothing to do
   elseif self._state == RUNNING or self._state == STARTED then
      self:stop()
   else
      assert(false, 'unknown state in destroy')
   end
   if self._action.destroy then
      self._action:destroy(self._ai_interface, self._entity)
   else
      self._log:spam('action does not implement destroy')
   end
   self:_set_state(DEAD)
   radiant.events.unpublish(self)   
end


function ExecutionUnitV2:start_thinking(current_entity_state)
   self._log:debug('start_thinking (state:%s processing:%s)', self._state, tostring(self._thinking))

   if self._thinking then
      self._log:spam('call to start_thinking while already thinking.  stopping...')
      self:stop_thinking()
      self._log:spam('resuming start_thinking, now that we stopped.')
   end
   
   assert(current_entity_state)
   assert(not self._thinking)
   
   self._thinking = true
   self._current_entity_state = current_entity_state
   self._ai_interface.CURRENT = current_entity_state
   
   self:_spam_current_state('before action start_thinking')  
   if not self._think_output then
      self:_set_state(THINKING)
      if self._action.start_thinking then
         self._action:start_thinking(self._ai_interface, self._entity, self._args)
      else
         self._log:spam('action does not implement start_thinking')
         self:__set_think_output(self._args)
      end
   else
      self._log:spam('ignoring start_thinking call.  already have args!')
   end
   self:_spam_current_state('after action start_thinking')
end

function ExecutionUnitV2:start()
   self._log:debug('start')
   assert(self:is_runnable())
   assert(self._state == READY)

   if self._action.start then      
      self._action:start(self._ai_interface, self._entity, self._args)
   else
      self._log:spam('action does not implement start')         
   end
   self:_set_state(STARTED)
end

function ExecutionUnitV2:run()
   self._log:debug('run')
   assert(self:get_priority() > 0)
   assert(self._state == STARTED)
   
   self:_set_state(RUNNING)
   local result = nil
   if self._action.run then
      self._log:debug('%s coroutine starting action: %s (%s)',
                      self._entity, self:get_name(), stonehearth.ai:format_args(self._think_output))
      result = { self._action:run(self._ai_interface, self._entity, self._args) }
      self._log:debug('%s coroutine finished: %s', self._entity, tostring(self:get_name()))
   else
      self._log:debug('action does not implement run.  (this is not an error, but is certainly weird)')
   end
   self:_set_state(FINISHED)
   return result
end

function ExecutionUnitV2:stop()
   self._log:debug('stop')
   
   if self:_in_state(READY, RUNNING, FINISHED) then
      self._log:debug('stop requested.')
      if self._execute_frame then
         self._execute_frame:destroy()
         self._execute_frame = nil
      end
      
      if self._action.stop then
         self._action:stop(self._ai_interface, self._entity, self._args)
      else
         self._log:spam('action does not implement stop')
      end
      self._current_entity_state = nil
      self:_set_state(IDLE)
   end
end
]]
-- helper methods

function ExecutionUnitV2:_verify_arguments(args, args_prototype)
   -- special case 0 vs nil so people can leave out .args and .think_output if there are none.
   args_prototype = args_prototype and args_prototype or {}
   if args[1] then
      self:__abort('activity arguments contains numeric elements (invalid!)')
      return false
   end
   if radiant.util.is_instance(args) then
      self:__abort('attempt to pass instance for arguments (not using associative array?)')
      return false
   end

   assert(not args[1], string.format('%s needs to convert to object instead of array passing!', self:get_name()))
   for name, value in pairs(args) do
      local expected_type = args_prototype[name]
      if not expected_type then
         self:__abort('unexpected argument "%s" passed to "%s".', name, self:get_name())
         return false
      end
      if expected_type ~= stonehearth.ai.ANY then
         if type(expected_type) == 'table' and not radiant.util.is_class(expected_type) then
            assert(expected_type.type, 'missing key "type" in complex args specified "%s"', name)
            expected_type = expected_type.type
         end
         if not radiant.util.is_a(value, expected_type) then
            self:__abort('wrong type for argument "%s" in "%s" (expected:%s got %s)', name,
                         self:get_name(), tostring(expected_type), tostring(type(value)))
            return false                      
         end
      end
   end
   for name, expected_type in pairs(args_prototype) do
      if args[name] == nil then
         if expected_type.default == stonehearth.ai.NIL then
            -- this one's ok.  keep going
         else
            if type(expected_type) == 'table' and not radiant.util.is_instance(expected_type) then
               args[name] = expected_type.default
            end
            if args[name] == nil then
               self:__abort('missing argument "%s" in "%s".', name, self:get_name())
               return false
            end
         end
      end
   end
   return true
end

function ExecutionUnitV2:_set_state(state)
   if self._state == ABORTED and self._state ~= DEAD then
      self._log:debug('ignoring state change to %s (currently %s)', tostring(state), self._state)
      return
   end
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   assert(self._state ~= DEAD)
   self._state = state
end

function ExecutionUnitV2:_spam_current_state(msg)
   self._log:spam(msg)
   if self._current_entity_state then
      self._log:spam('  CURRENT is %s', tostring(self._current_entity_state))
      for key, value in pairs(self._current_entity_state) do      
         self._log:spam('  CURRENT.%s = %s', key, tostring(value))
      end   
   else
      self._log:spam('  no CURRENT state!')
   end
end

return ExecutionUnitV2
