local ExecutionFrame = require 'components.ai.execution_frame'
local ExecutionUnitV2 = class()

local THINKING = 'thinking'
local READY = 'ready'
local STARTED = 'started'
local RUNNING = 'running'
local HALTED = 'halted'
local STOPPED = 'stopped'
local DEAD = 'dead'
local ABORTED = 'aborted'
local placeholders = require 'services.ai.placeholders'

function ExecutionUnitV2._add_nop_state_transition(msg, state)
   local method = '_' .. msg .. '_from_' .. state
   assert(not ExecutionUnitV2[method])
   ExecutionUnitV2[method] = function() end
end

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
                               :set_user_data(self)
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
      self._thread:suspend('ex_unit is idle')
   end
end

function ExecutionUnitV2:_dispatch_msg(msg, ...)
   local method = '_' .. msg .. '_from_' .. self._state
   local dispatch_msg_fn = self[method]
   
   if not dispatch_msg_fn and self._state == 'ready' then
      self._log:detail('no %s handler.  trying to remap to the thinking handler.', method)
      -- cheat a little bit... there's almost no distinction how transitions from 'ready' and 'thinking'
      -- are handled (with the execption of start)...  so if there's not a handler for the current
      -- msg from the ready state, try the thinking one.  that way we don't have to duplicate code
      dispatch_msg_fn = self['_' .. msg .. '_from_thinking']
   end

   assert(dispatch_msg_fn, string.format('unknown state transition in execution frame "%s"', method))
   dispatch_msg_fn(self, ...)
end

function ExecutionUnitV2:_call_action(method)
   local call_action_fn = self._action[method]
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
   self:_set_state(HALTED)
   self._parent_thread:send_msg('unit_halted', self)
end

function ExecutionUnitV2:_stop_from_thinking()
   assert(self._thinking)

   self:_stop_thinking()
   self:_call_action('stop')
   self:_set_state(STOPPED)
   self._parent_thread:send_msg('unit_stopped', self)
end

function ExecutionUnitV2:_stop_from_halted()
   assert(not self._thinking)

   self:_call_action('stop')
   self:_set_state(STOPPED)
   self._parent_thread:send_msg('unit_stopped', self)
end

function ExecutionUnitV2:_shutdown_from_stopped()
   assert(not self._thinking)
   assert(not self._execute_frame)

   self:_call_action('destroy')
   self:_terminate('shutting down')
end

function ExecutionUnitV2:_terminate_from_running(reason)
   assert(not self._thinking)

   self:_set_state(DEAD)
   if self._execute_frame then
      self._execute_frame:send_msg('terminate', reason)
   end
   self:_call_action('destroy')
end

function ExecutionUnitV2:_child_thread_exit_from_running(thread, err)
   local frame = thread:get_user_data()
   assert(frame == self._execute_frame)
   if err then
      self._log:debug('execution frame died while running (%s)', err)
      self._terminate(err)
   end
end

-- terminate is private because it's only valid to call it on our thread
-- after we've called stop_thinking, stop, and destroy on the action
function ExecutionUnitV2:_terminate(reason)
   -- terminate to ensure that we process no more messeages.
   assert(self._thread:is_running())
   self._thread:terminate(reason)
   radiant.not_reached()
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
   self._execute_frame:shutdown()
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
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__resume called (reason: %s)', reason)

   assert(not self._thread:is_running())
   self._thread:resume(reason)
end

function ExecutionUnitV2:__abort(reason, ...)
   local reason = reason and string.format(reason, ...) or 'no reason given'
   self._log:debug('__abort %s called (state: %s)', tostring(reason), self._state)

   if self._thread:is_running() then
      -- abort is not allowed to return.  evar!  so if the thread is currently
      -- running, we have no choice but to abort it.  if we ever leave this
      -- function, we'll break the 'abort does not return' contract we have with
      -- the action      
      self._thread:terminate('aborted: ' .. reason)
      radiant.not_reached('abort does not return')
   else
      self._thread:send_msg('terminate', reason)
   end
end

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

function ExecutionUnitV2:in_state(...)
   for _, state in ipairs({...}) do
      if self._state == state then
         return true
      end
   end
   return false
end

function ExecutionUnitV2:_set_state(state)
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   assert(self._state ~= DEAD)
   self._state = state
   if self._state == DEAD then
      self._thread:resume('time to die...')
   end
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

local ANY_STATE = 'any_state'
ExecutionUnitV2._add_nop_state_transition('stop', STOPPED)
ExecutionUnitV2._add_nop_state_transition('stop_thinking', STOPPED)
ExecutionUnitV2._add_nop_state_transition('child_thread_exit', ANY_STATE)

return ExecutionUnitV2
