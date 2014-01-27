local ExecutionUnitV2 = class()

local THINKING = 'thinking'
local READY = 'ready'
local STARTED = 'started'
local RUNNING = 'running'
local FINISHED = 'finished'
local STOPPED = 'stopped'
local DEAD = 'dead'
local ABORTED = 'aborted'
local placeholders = require 'services.ai.placeholders'

function ExecutionUnitV2:__init(frame, thread, debug_route, entity, injecting_entity, action, args, action_index)
   assert(action.name)
   assert(action.does)
   assert(action.args)
   assert(action.priority)

   self._id = stonehearth.ai:get_next_object_id()
   self._frame = frame
   self._thread = thread
   self._debug_route = debug_route .. ' u:' .. tostring(self._id)
   self._entity = entity
   self._action = action
   self._args = args
   self._action_index = action_index

   self._think_output_types = self._action.think_output or self._action.args
     
   self._ai_interface = { ___execution_unit = self }   
   local chain_function = function (fn)
      self._ai_interface[fn] = function(i, ...) return self['__'..fn](self, ...) end
   end
   chain_function('set_think_output')
   chain_function('clear_think_output')
   chain_function('spawn')
   chain_function('execute')   
   chain_function('suspend')
   chain_function('resume')
   chain_function('abort')
   chain_function('get_log')   
  
   local actions = {
      'start_thinking',
      'stop_thinking',
      'start',
      'stop',
      'run',
      'destroy'
   }
   for _, method in ipairs(actions) do
      ExecutionUnitV2['_call_' .. method] = function (self)
         local call_action_fn = self._action[method]
         if call_action_fn then
            self._calling_method = method
            self._log:detail('calling action %s', method)
            call_action_fn(self._action, self._ai_interface, self._entity, self._args)
            self._calling_method = nil
         else
            self._log:detail('action does not implement %s.', method)
         end
         return call_action_fn ~= nil
      end
   end


   self._log = radiant.log.create_logger('ai.exec_unit')
   local prefix = string.format('%s (%s)', self._debug_route, self:get_name())
   self._log:set_prefix(prefix)
   self._log:debug('creating execution unit')
   
   if self:_verify_arguments(args, self._action.args) then
      self:_set_state(STOPPED)
   else
      assert(false, 'put ourselves in the dead state')
   end
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

function ExecutionUnitV2:_unknown_transition(msg)
   error(string.format('unknown state transition in execution unit "%s" from_state "%s"', msg, self._state))
end

function ExecutionUnitV2:_start_thinking(entity_state)
   if self._state == 'stopped' then
      return self:_start_thinking_from_stopped(entity_state)
   end
   self:_unknown_transition('start_thinking')   
end

function ExecutionUnitV2:_set_think_output(think_output)
   if self._state == 'thinking' then
      return self:_set_think_output_from_thinking(think_output)
   end
   if self._state == 'ready' then
      return self:_set_think_output_from_ready(think_output)
   end
   self:_unknown_transition('set_think_output')   
end

function ExecutionUnitV2:_clear_think_output()
   if self:in_state('thinking', 'ready') then
      return self:_clear_think_output_from_thinking()
   end
   if self._state == 'finished' then
      return self:_clear_think_output_from_finished()
   end
   self:_unknown_transition('clear_think_output')   
end

function ExecutionUnitV2:_stop_thinking()
   if self:in_state('thinking', 'ready') then
      return self:_stop_thinking_from_thinking()
   end
   if self._state == 'started' then
      return self:_stop_thinking_from_started()
   end
   self:_unknown_transition('stop_thinking')   
end

function ExecutionUnitV2:_start()
   if self._state == 'ready' then
      return self:_start_from_ready()
   end
   self:_unknown_transition('start')   
end

function ExecutionUnitV2:_run()
   if self._state == 'ready' then
      return self:_run_from_ready()
   end
   if self._state == 'started' then
      return self:_run_from_started()
   end
   self:_unknown_transition('run')   
end

function ExecutionUnitV2:_stop(invalid_transition_ok)
   if self:in_state('thinking', 'ready') then
      return self:_stop_from_thinking(invalid_transition_ok)
   end
   if self._state == 'running' then
      return self:_stop_from_running(invalid_transition_ok)
   end
   if self._state == 'finished' then
      return self:_stop_from_finished(invalid_transition_ok)
   end
   if self._state == 'stopped' or self._state == 'stop_thinking' then
      return -- nopf
   end
   self:_unknown_transition('stop')   
end

function ExecutionUnitV2:_destroy()
   if self:in_state('thinking', 'ready') then
      return self:_destroy_from_thinking()
   end
   if self._state == 'stopped' then
      return self:_destroy_from_stopped()
   end
   if self._state == 'finished' then
      return self:_destroy_from_finished()
   end
   self:_unknown_transition('destroy')   
end

function ExecutionUnitV2:_start_thinking_from_stopped(entity_state)
   assert(not self._thinking)

   self:_set_state(THINKING)
   self:_do_start_thinking(entity_state)
end

function ExecutionUnitV2:_set_think_output_from_thinking(think_output)
   if self._state == DEAD then
      self._log:debug('ignoring set_think_output in dead state')
      return
   end

   if not think_output then
      think_output = self._args
   end
   self:_verify_arguments(think_output, self._think_output_types)

   self:_set_state(READY)
   self._frame:_unit_ready(self, think_output)
end

function ExecutionUnitV2:_set_think_output_from_ready(think_output)
   self._log:info('action called set_think_output from ready state.  using previous think output!')
   return
end

function ExecutionUnitV2:_clear_think_output_from_thinking()
   if self._state == DEAD then
      self._log:debug('ignoring clear_think_output in dead state')
      return
   end

   self:_set_state(THINKING)
   self._frame:_unit_not_ready(self)
end

function ExecutionUnitV2:_clear_think_output_from_finished()
   -- who cares?
end

function ExecutionUnitV2:_stop_thinking_from_thinking()
   assert(self._thinking)
   self:_do_stop_thinking()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_stop_thinking_from_started()
   if self._thinking then
      self:_do_stop_thinking()
   end
   -- stay in the started stated.
end

function ExecutionUnitV2:_start_from_ready()
   assert(self._thinking)

   self:_do_start()
   self:_set_state(STARTED)
   self:_do_stop_thinking()
end

function ExecutionUnitV2:_run_from_ready()
   assert(self._thinking)

   self:_do_start()
   self:_set_state(RUNNING)
   self:_do_stop_thinking()
   self:_call_run()
   self:_set_state(FINISHED)
end

function ExecutionUnitV2:_run_from_started()
   assert(not self._thinking)

   self:_set_state(RUNNING)
   self:_call_run()
   self:_set_state(FINISHED)
end

function ExecutionUnitV2:_stop_from_thinking()
   assert(self._thinking)

   self:_do_stop_thinking()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_stop_from_running(frame_will_kill_running_action)
   assert(not self._thinking)
   assert(self._thread:is_running())

   -- assume our calling frame knows what it's doing.  if not,
   -- we're totally going to get screwed
   assert(frame_will_kill_running_action)
   
   -- our execute frame must be dead and buried if our owning frame has the
   -- audicity to kill us while running
   if self._execute_frame then
      assert(self._execute_frame:get_state() == 'dead')
      self._execute_frame =  nil
   end
   self:_do_stop()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_stop_from_finished()
   assert(not self._thinking)

   self:_do_stop()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_destroy_from_thinking()
   assert(self._thinking)
   assert(not self._execute_frame)
   self:_call_stop_thinking()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_stopped()
   assert(not self._thinking)
   assert(not self._execute_frame)
   self:_call_destroy()
   self:_set_state(DEAD)
end
function ExecutionUnitV2:_destroy_from_finished()
   assert(not self._thinking)
   assert(not self._execute_frame)
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_do_start()
   self._log:debug('_do_start (state:%s)', tostring(self._state))
   assert(self._thinking)
   assert(not self._started)
   
   self._started = true
   self:_call_start()
end

function ExecutionUnitV2:_do_stop()
   self._log:debug('_do_stop (state:%s)', tostring(self._state))
   assert(not self._thinking)
   assert(self._started, '_do_stop called before start')
   
   self._started = false
   self:_call_stop()
end

function ExecutionUnitV2:_do_start_thinking(entity_state)
   self._log:debug('_do_start_thinking (state:%s)', tostring(self._state))
   assert(not self._thinking)
   
   self._thinking = true
   self._current_entity_state = entity_state
   self._ai_interface.CURRENT = entity_state

   if not self:_call_start_thinking() then
      self:_set_think_output(nil)
   end
end

function ExecutionUnitV2:_do_stop_thinking()
   self._log:debug('_do_stop_thinking (state:%s)', tostring(self._state))
   assert(self._thinking)

   self._current_entity_state = nil
   self._ai_interface.CURRENT = nil
   self._thinking = false
   
   self:_call_stop_thinking()
end

-- the interface facing actions (i.e. the 'ai' interface)
function ExecutionUnitV2:__get_log()
   return self._log
end

function ExecutionUnitV2:__execute(name, args)
   self._log:debug('__execute %s %s called', name, stonehearth.ai:format_args(args))
   assert(self._thread:is_running())
   assert(not self._execute_frame)

   local ExecutionFrame = require 'components.ai.execution_frame'
   self._execute_frame = ExecutionFrame(self._thread, self._debug_route, self._entity, name, args, self._action_index)
   self._execute_frame:run()
   self._execute_frame:stop()
   self._execute_frame:destroy()
   self._execute_frame = nil
end 

function ExecutionUnitV2:__set_think_output(args)
   self._log:debug('__set_think_output %s called', stonehearth.ai:format_args(args))
   if self._in_start_thinking then
      -- wow, so eager!  don't send a message, just hold onto the answer for when we rewind...
      self._reentrant_think_output = args
   else
      self:_set_think_output(args)
   end
end

function ExecutionUnitV2:__clear_think_output(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:debug('__clear_think_output called (reason: %s)', reason)
   self:_clear_think_output()
end

function ExecutionUnitV2:__spawn(name, args)   
   self._log:debug('__spawn %s %s called', name, stonehearth.ai:format_args(args))
  
   local ExecutionFrame = require 'components.ai.execution_frame'  
   return ExecutionFrame(self._thread, self._debug_route, self._entity, name, args, self._action_index)
end

function ExecutionUnitV2:__suspend(format, ...)
   assert(self._thread:is_running(), 'cannot call ai:suspend() when not running')
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__suspend called (reason: %s)', reason)
   self._thread:suspend(reason)

   -- we should never allow the thread to comeback in the DEAD state.  that means
   -- we've been termianted when suspended, and shouldn't be running at all!
   assert (self._state ~= DEAD, string.format('thread resumed into DEAD state.'))
end

function ExecutionUnitV2:__resume(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__resume called (reason: %s)', reason)

   if self._state == DEAD then
      self._log:debug('ignoring call to resume in dead state')
   else
      self._thread:resume(reason)
   end
end

function ExecutionUnitV2:__abort(reason, ...)
   local reason = reason and string.format(reason, ...) or 'no reason given'
   self._log:debug('__abort %s called (state: %s)', tostring(reason), self._state)

   if self._thread:is_running() then
      -- abort is not allowed to return.  evar!  so if the thread is currently
      -- running, we have no choice but to abort it.  if we ever leave this
      -- function, we'll break the 'abort does not return' contract we have with
      -- the action
      self._frame:abort()
      radiant.not_reached('abort does not return')
   else
      error('need to implement abort while not running')
   end
end

function ExecutionUnitV2:_verify_arguments(args, args_prototype)
   -- special case 0 vs nil so people can leave out .args and .think_output if there are none.
   args_prototype = args_prototype and args_prototype or {}
   if args[1] then
      error('activity arguments contains numeric elements (invalid!)')
      return false
   end
   if radiant.util.is_instance(args) then
      error('attempt to pass instance for arguments (not using associative array?)')
      return false
   end

   assert(not args[1], string.format('%s needs to convert to object instead of array passing!', self:get_name()))
   for name, value in pairs(args) do
      local expected_type = args_prototype[name]
      if not expected_type then
         error(string.format('unexpected argument "%s" passed to "%s".', name, self:get_name()))
         return false
      end
      if expected_type ~= stonehearth.ai.ANY then
         if type(expected_type) == 'table' and not radiant.util.is_class(expected_type) then
            assert(expected_type.type, 'missing key "type" in complex args specified "%s"', name)
            expected_type = expected_type.type
         end
         if not radiant.util.is_a(value, expected_type) then
            error(string.format('wrong type for argument "%s" in "%s" (expected:%s got %s)', name,
                                self:get_name(), tostring(expected_type), tostring(type(value))))
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
               error(string.format('missing argument "%s" in "%s".', name, self:get_name()))
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

function ExecutionUnitV2:get_state(state)
   return self._state
end

function ExecutionUnitV2:_set_state(state)
   self._log:debug('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)

   if self._state ~= DEAD then
      self._state = state   
   else
      self._log:debug('cannot transition to %s from DEAD.  remaining dead', state)
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

return ExecutionUnitV2
