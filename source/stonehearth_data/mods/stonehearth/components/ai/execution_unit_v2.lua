local Entity = _radiant.om.Entity

local ExecutionUnitV2 = class()

local THINKING = 'thinking'
local READY = 'ready'
local STARTED = 'started'
local STARTING = 'starting'
local RUNNING = 'running'
local STOPPING = 'stopping'
local FINISHED = 'finished'
local STOPPED = 'stopped'
local DEAD = 'dead'

local CALL_NOT_IMPLEMENTED = {}

local placeholders = require 'services.server.ai.placeholders'
local ObjectMonitor = require 'components.ai.object_monitor'

local ENTITY_STATE_FIELDS = {
   location = true,
   carrying = true,
}

function ExecutionUnitV2:__init(frame, thread, debug_route, entity, injecting_entity, action, action_index, trace_route)
   assert(action.name)
   assert(action.does)
   assert(action.args)
   assert(action.priority)

   self._id = stonehearth.ai:get_next_object_id()
   self._cost = 0
   self._frame = frame
   self._thread = thread
   self._debug_name = string.format("[u:%d %s]", self._id, action.name)
   self._debug_route = debug_route .. ' u:' .. tostring(self._id)
   self._trace_route = trace_route .. tostring(self._id) .. '/'
   self._entity = entity
   self._ai_component = entity:get_component('stonehearth:ai')
   self._action = action
   self._action_index = action_index
   self._execution_frames = {}
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
   chain_function('set_status_text')
   chain_function('set_cost')
   chain_function('protect_entity')
   chain_function('unprotect_entity')
  
   local actions = {
      -- for filters only   
      'should_start_thinking',

      -- for actions only
      'start_thinking',             
      'stop_thinking',
      'start',
      'stop',
      'run',
      'destroy'
   }
   for _, method in ipairs(actions) do
      ExecutionUnitV2['_call_' .. method] = function (self)
         local result
         local call_action_fn = self._action[method]
         if call_action_fn then
            self._calling_method = method
            self._log:detail('calling action %s', method)
            result = call_action_fn(self._action, self._ai_interface, self._entity, self._args)
            self._calling_method = nil
         else
            self._log:detail('action does not implement %s.', method)
            result = CALL_NOT_IMPLEMENTED
         end
         return result
      end
   end

   local prefix = string.format('%s (%s)', self._debug_route, self:get_name())
   self._log = radiant.log.create_logger('ai.exec_unit')
                          :set_prefix(prefix)
                          :set_entity(self._entity)

   self._log:debug('creating execution unit')
   self._aitrace = radiant.log.create_logger('ai_trace')
   
   -- Set the trace-route initially to the old route, so that in logging it is clear that the
   -- frame is creating this unit.
   self._aitrace:set_prefix(trace_route)   
   self._aitrace:spam('@ceu@%d@%s', self._id, self:get_name())
   self._aitrace:set_prefix(self._trace_route)   
   

   self:_set_state(STOPPED)
end

function ExecutionUnitV2:get_action_interface()
   return self._ai_interface
end

function ExecutionUnitV2:get_action()
   return self._action
end

-- ExecutionFrame facing functions...
function ExecutionUnitV2:get_name()
   return self._debug_name
end

function ExecutionUnitV2:get_cost()
   return self._cost
end

function ExecutionUnitV2:get_id()
   return self._id
end

function ExecutionUnitV2:get_priority()
   return self._action.priority
end

function ExecutionUnitV2:get_weight()
   return self._action.weight or 1
end

function ExecutionUnitV2:is_runnable()
   return self._action.priority > 0 and self:in_state(READY, RUNNING)
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
      id = 'u:' .. tostring(self._id),
      state = self._state,
      action = self:_get_action_debug_info(),
      think_output = stonehearth.ai:format_args(self._think_output),
      execution_frames = {
         n = 0,
      }
   }
   if self._current_execution_frame then
      table.insert(info.execution_frames, self._current_execution_frame:get_debug_info())
   end
   return info
end

function ExecutionUnitV2:_unknown_transition(msg)
   local err = string.format('bad unit transition "%s" from  "%s"', msg, self._state)
   self._log:info(err)
   error(err)
end

-- called only for filters...
function ExecutionUnitV2:_should_start_thinking(args, entity_state)
   assert(self._state == 'stopped')

   if not self:_verify_arguments('start_thinking', args, self._action.args) then
      return false
   end

   self._args = args
   self._current_entity_state = entity_state
   self._ai_interface.CURRENT = entity_state

   return self:_call_should_start_thinking()
end

function ExecutionUnitV2:_start_thinking(args, entity_state)
   assert(args, '_start_thinking called with no args')
   assert(entity_state, '_start_thinking called with no entity_state')

   -- verify the arguments are correct
   if not self:_verify_arguments('start_thinking', args, self._action.args) then      
      self:_stop()
      return
   end
   self._args = args

   if self:in_state(DEAD) then
      self._log:detail('ignoring "start_thinking" in state "%s"', self._state)
      return
   end

   if self:in_state(THINKING, READY) then
      return self:_start_thinking_from_thinking(entity_state)
   end

   if self._state == 'stopped' then
      return self:_start_thinking_from_stopped(entity_state)
   end
   self:_unknown_transition('start_thinking')   
end

function ExecutionUnitV2:_set_think_output(think_output)
   if self:in_state(DEAD) then
      self._log:detail('ignoring "set_think_output" in state "%s"', self._state)
      return
   end

   if self._state == 'thinking' then
      return self:_set_think_output_from_thinking(think_output)
   end
   if self._state == 'ready' then
      return self:_set_think_output_from_ready(think_output)
   end
   self:_unknown_transition('set_think_output')   
end

function ExecutionUnitV2:_clear_think_output()
   if self:in_state(DEAD) then
      self._log:detail('ignoring "clear_think_output" in state "%s"', self._state)
      return
   end

   if self:in_state('thinking', 'ready') then
      return self:_clear_think_output_from_thinking()
   end
   if self._state == 'finished' then
      return -- who cares?
   end
   self:_unknown_transition('clear_think_output')   
end

function ExecutionUnitV2:_stop_thinking()
   if self:in_state(DEAD) then
      self._log:detail('ignoring "stop_thinking" in state "%s"', self._state)
      return
   end

   if self:in_state('thinking', 'ready') then
      return self:_stop_thinking_from_thinking()
   end
   if self:in_state('starting', 'started') then
      return self:_stop_thinking_from_started()
   end
   if self:in_state('stopping', 'stopped', 'dead') then
      assert(not self._thinking)
      return -- nop
   end
   self:_unknown_transition('stop_thinking')   
end

function ExecutionUnitV2:_start()
   if self:in_state(DEAD) then
      self._log:detail('ignoring "start" in state "%s"', self._state)
      return
   end

   if self._state == 'ready' then
      return self:_start_from_ready()
   end
   self:_unknown_transition('start')   
end

function ExecutionUnitV2:_run()
   if self:in_state(DEAD) then
      self._log:detail('ignoring "run" in state "%s"', self._state)
      return
   end

   if self._state == 'ready' then
      return self:_run_from_ready()
   end
   if self._state == 'started' then
      return self:_run_from_started()
   end
   self:_unknown_transition('run')   
end

function ExecutionUnitV2:_stop()
   self:_destroy_object_monitor()
   
   if self:in_state(DEAD) then
      self._log:detail('ignoring "stop" in state "%s"', self._state)
      return
   end

   if self:in_state('thinking', 'ready') then
      return self:_stop_from_thinking()
   end
   if self._state == 'starting' then
      return self:_stop_from_starting()
   end
   if self._state == 'started' then
      return self:_stop_from_started()
   end
   if self._state == 'running' then
      return self:_stop_from_running()
   end
   if self._state == 'finished' then
      return self:_stop_from_finished()
   end
   if self:in_state('stopped', 'stopping', 'stop_thinking') then
      return -- nop
   end
   self:_unknown_transition('stop')   
end

function ExecutionUnitV2:_destroy()
   self._log:detail('destroy')

   self:_destroy_object_monitor()
   
   if self:in_state('thinking', 'ready') then
      return self:_destroy_from_thinking()
   end
   if self._state == 'starting' then
      return self:_destroy_from_starting()
   end
   if self._state == 'stopping' then
      return self:_destroy_from_stopping()
   end
   if self._state == 'stopped' then
      return self:_destroy_from_stopped()
   end
   if self._state == 'running' then
      return self:_destroy_from_running()
   end
   if self._state == 'finished' then
      return self:_destroy_from_finished()
   end
   if self._state == 'dead' then
      return -- nop
   end
   self:_unknown_transition('destroy')   
end

function ExecutionUnitV2:_start_thinking_from_stopped(entity_state)
   assert(not self._thinking)
   assert(not self._object_monitor)

   self:_set_state(THINKING)

   -- create an object monitor to keep track of all the entities required by
   -- this action.
   self._object_monitor = self:_create_object_monitor()
   if not self._object_monitor then
      self._log:detail('failed to monitor all entities in args.  bailing.')
      self:_set_state(STOPPED)
      return
   end

   self:_do_start_thinking(entity_state)
end

function ExecutionUnitV2:_start_thinking_from_thinking(entity_state)
   assert(self._thinking)
   assert(self._object_monitor)

   self:_do_stop_thinking()
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
   self:_verify_arguments('set_think_output', think_output, self._think_output_types)

   assert(self._object_monitor)
   self._log:detail('pausing object monitor in _set_think_output_from_thinking')
   self._object_monitor:pause()

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

function ExecutionUnitV2:_stop_thinking_from_thinking()
   assert(self._thinking)
   self:_do_stop_thinking()
   self:_destroy_object_monitor()
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

   -- start is called before stop_thinking so actions will know whether
   -- they're going to get to run...
   self:_do_start()
   self:_do_stop_thinking()
end

function ExecutionUnitV2:_run_from_ready()
   assert(self._thinking)
   assert(self._object_monitor)
   assert(not self._object_monitor:is_running()) -- will be resumed by do_start

   self:_do_start()
   self:_set_state(RUNNING)
   self:_do_stop_thinking()
   self:_call_run()
   self:_destroy_object_monitor()
   self:_set_state(FINISHED)
end

function ExecutionUnitV2:_run_from_started()
   assert(not self._thinking)
   assert(self._object_monitor)
   assert(self._object_monitor:is_running())

   self:_set_state(RUNNING)
   self:_call_run()
   self:_destroy_object_monitor()   
   self:_set_state(FINISHED)
end

function ExecutionUnitV2:_stop_from_thinking()
   assert(self._thinking)

   self:_do_stop_thinking()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_stop_from_starting()
   if self._thinking then
      self:_do_stop_thinking()
   end
   self:_do_stop()
end

function ExecutionUnitV2:_stop_from_started()
   assert(not self._thinking)
   self:_do_stop()
end

function ExecutionUnitV2:_stop_from_finished()
   assert(not self._thinking)
   self:_do_stop()
end

function ExecutionUnitV2:_stop_from_running()
   assert(not self._thinking)
   assert(self._thread:is_running() or stonehearth.threads:get_current_thread() == nil)

   if self._current_execution_frame then
      self._current_execution_frame:stop(true)
      self._current_execution_frame = nil
   end  
   self:_do_stop()
end

function ExecutionUnitV2:_destroy_from_thinking()
   assert(self._thinking)
   assert(not self._current_execution_frame)
   self:_destroy_execution_frames()
   self:_call_stop_thinking()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_starting()
   assert(self._thinking)
   assert(not self._current_execution_frame)
   self:_destroy_execution_frames()

   if self._thinking then
      self:_call_stop_thinking()
   end
   if self._started then
      self:_call_stop()
   end
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_stopping()
   assert(not self._thinking)
   assert(not self._current_execution_frame)
   self:_destroy_execution_frames()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_stopped()
   assert(not self._thinking)
   assert(not self._current_execution_frame)
   self:_destroy_execution_frames()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_running()
   assert(not self._thinking)
   self:_destroy_execution_frames()
   self:_call_stop()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_destroy_from_finished()
   assert(not self._thinking)
   assert(not self._current_execution_frame)
   self:_destroy_execution_frames()
   self:_call_destroy()
   self:_set_state(DEAD)
end

function ExecutionUnitV2:_do_start()
   self._log:debug('_do_start (state:%s)', tostring(self._state))
   assert(self._thread:is_running())
   assert(self._thinking)
   assert(not self._started)
   assert(self._object_monitor)
   assert(not self._object_monitor:is_running())

   if self._action.status_text then
      self._ai_component:set_status_text(self._action.status_text)
   end

   -- switch to the abort version
   self._object_monitor:set_destroyed_cb(function(name)
         self._log:detail('%s was destroyed during run.  aborting', name)
         self:_destroy_object_monitor()
         self._thread:unsafe_interrupt(function()
               self._frame:abort()
               radiant.not_reached('abort does not return')
            end)
         -- if we made it here, unsafe_interrupt() could not deliver our cb.
         -- we have no choice but to keep running!
         radiant.log.write('stonehearth', 0, '"%s" action, failed to abort on entity destruction.' ..
                                             '  did you forget to unprotect it?', self._action.name)
      end)
   self._object_monitor:resume()

   self:_set_state(STARTING)  
   self._started = true
   self:_call_start()
   self:_set_state(STARTED)
end

function ExecutionUnitV2:_do_stop()
   self._log:debug('_do_stop (state:%s)', tostring(self._state))
   assert(not self._current_execution_frame)
   assert(not self._thinking)
   assert(self._started, '_do_stop called before start')
   
   self:_set_state(STOPPING)
   self._started = false
   self:_call_stop()
   self:_set_state(STOPPED)
end

function ExecutionUnitV2:_do_start_thinking(entity_state)
   assert(entity_state, '_do_start_thinking called with no entity_state')
   self._log:debug('_do_start_thinking (state:%s)', tostring(self._state))
   assert(not self._thinking)

   self:_verify_entity_state(entity_state)
   
   self._cost = self._action.cost or 0
   self._thinking = true
   self._current_entity_state = entity_state
   self._ai_interface.CURRENT = entity_state
   if self:_call_start_thinking() == CALL_NOT_IMPLEMENTED then
      self:_set_think_output(nil)
   end
end

function ExecutionUnitV2:_verify_entity_state(entity_state)
   for name in pairs(entity_state) do
      assert(ENTITY_STATE_FIELDS[name] == true,
             string.format('invalid field "%s" in ai.CURRENT (entity_state) after calling start_thinking on "%s"', name, self._action.name))
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

function ExecutionUnitV2:_destroy_execution_frames()
   for _, frame in pairs(self._execution_frames) do
      frame:destroy()
   end
   self._execution_frames = {}

   self._current_execution_frame = nil
end

function ExecutionUnitV2:__get_log()
   return self._log
end

function ExecutionUnitV2:__set_cost(cost)
   assert(cost)
   self._cost = cost
end

function ExecutionUnitV2:__protect_entity(entity)
   if entity and entity:is_valid() and self._object_monitor then
      self._object_monitor:protect_object(entity)
   end
end

function ExecutionUnitV2:__unprotect_entity(entity)
   if entity and entity:is_valid() and self._object_monitor then
      self._object_monitor:unprotect_object(entity)
   end
end

function ExecutionUnitV2:__set_status_text(format, ...)
   self._ai_component:set_status_text(string.format(format, ...))
end

function ExecutionUnitV2:__execute(activity_name, args)
   self._log:debug('__execute %s %s called', activity_name, stonehearth.ai:format_args(args))
   assert(self._thread:is_running())
   assert(not self._current_execution_frame)

   self._current_execution_frame = self._execution_frames[activity_name]
   if not self._current_execution_frame then
      self._current_execution_frame = self._ai_component:create_execution_frame(activity_name, self._debug_route, self._trace_route)
      self._execution_frames[activity_name] = self._current_execution_frame
   end

   assert(self._current_execution_frame)
   assert(self._current_execution_frame:get_state() == STOPPED)

   self._current_execution_frame:run(args or {})
   self._current_execution_frame:stop()
   self._current_execution_frame = nil
end 

function ExecutionUnitV2:__set_think_output(args)
   self._aitrace:spam('@o@%s', stonehearth.ai:format_args(args))
   self._log:debug('__set_think_output %s called', stonehearth.ai:format_args(args))
   self:_set_think_output(args)
end

function ExecutionUnitV2:__clear_think_output(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:debug('__clear_think_output called (reason: %s)', reason)
   self:_clear_think_output()
end

function ExecutionUnitV2:__spawn(activity_name)
   self._log:debug('__spawn %s called', activity_name)
  
   return self._ai_component:create_execution_frame(activity_name, self._debug_route, self._trace_route)
end

function ExecutionUnitV2:__suspend(format, ...)
   assert(self._state == RUNNING, 'cannot call ai:suspend() outside of your :run() method')
   assert(self._thread:is_running(), 'cannot call ai:suspend() when thread is not running')
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__suspend called (reason: %s)', reason)
   self._thread:suspend(reason)

   -- we should never allow the thread to comeback in the DEAD state.  that means
   -- we've been termianted when suspended, and shouldn't be running at all!
   assert (not self:in_state(DEAD), string.format('thread resumed into %s state.', self._state))
end

function ExecutionUnitV2:__resume(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('__resume called (reason: %s)', reason)

   if self:in_state(DEAD) then
      self._log:debug('ignoring call to resume in %s state', self._state)
   else
      self._thread:resume(reason)
   end
end

function ExecutionUnitV2:__abort(reason, ...)
   local reason = reason and string.format(reason, ...) or 'no reason given'
   self._log:warning('__abort %s called (state: %s)', tostring(reason), self._state)

   if self._thread:is_running() then
      -- abort is not allowed to return.  evar!  so if the thread is currently
      -- running, we have no choice but to abort it.  if we ever leave this
      -- function, we'll break the 'abort does not return' contract we have with
      -- the action
      self._frame:abort()
      radiant.not_reached('abort does not return')
   else
      self._log:debug('not on the right thread to abort.  thunking over...')
      self._thread:interrupt(function()
            self._frame:abort()
            radiant.not_reached('abort does not return')
         end)
   end
end

function ExecutionUnitV2:_verify_arguments(fname, args, args_prototype)
   local function report_error(format, ...)
      error(string.format('error in args passed to ' .. fname .. ': ' .. format, ...))
   end
   
   -- special case 0 vs nil so people can leave out .args and .think_output if there are none.
   args_prototype = args_prototype and args_prototype or {}
   if args[1] then
      report_error('activity arguments contains numeric elements (invalid!)')
      return false
   end
   if radiant.util.is_instance(args) then
      report_error('attempt to pass instance for arguments (not using associative array?)')
      return false
   end

   assert(not args[1], string.format('%s needs to convert to object instead of array passing!', self:get_name()))
   for name, value in pairs(args) do
      local expected_type = args_prototype[name]
      if not expected_type then
         report_error('unexpected argument "%s" passed to "%s".', name, self:get_name())
         return false
      end
      if expected_type ~= stonehearth.ai.ANY then
         if type(expected_type) == 'table' and not radiant.util.is_class(expected_type) then
            assert(expected_type.type, 'missing key "type" in complex args specified "%s"', name)
            expected_type = expected_type.type
         end
         if not radiant.util.is_a(value, expected_type) then
            report_error('wrong type for argument "%s" in "%s" (expected:%s got %s)', name,
                                self:get_name(), tostring(expected_type), tostring(type(value)))
            return false                      
         end
      end
   end
   for name, expected_type in pairs(args_prototype) do
      if args[name] == nil then
         if type(expected_type) == 'table' then
            if radiant.util.is_class(expected_type) then
               -- we're totally missing the argument!  bail
               report_error('missing argument "%s" of type "%s" in "%s".', name, radiant.util.tostring(expected_type), self:get_name())
               return false
            elseif expected_type.default ~= nil then
               -- maybe there's a default?
               if expected_type.default == stonehearth.ai.NIL then
                  -- this one's ok.  keep going
               else
                  args[name] = expected_type.default
               end
            end
         else
            -- we're totally missing the argument!  bail
            report_error('missing argument "%s" of type "%s" in "%s".', name, radiant.util.tostring(expected_type), self:get_name())
            return false
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

   if self._state ~= STARTING and self._state ~= STARTED then
      self._aitrace:spam('@sc@%s@%s', tostring(self._state), state)
   end

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

function ExecutionUnitV2:_create_object_monitor()
   assert(self._args)
   assert(self._state == THINKING)

   if self._action.unprotected_args then
      self._log:info('by protecting entities on action\'s request')
      return ObjectMonitor(self._log)
   end
   
   -- run through the arguments once looking for bad ones.
   for _, obj in pairs(self._args) do
      if radiant.util.is_a(obj, Entity) and not obj:is_valid() then
         return nil
      end
   end

   -- all good!  create the object monitor
   self._log:detail('creating object monitor')
   local object_monitor = ObjectMonitor(self._log)
   for _, obj in pairs(self._args) do
      if radiant.util.is_a(obj, Entity) then
         object_monitor:protect_object(obj)
      end
   end
   object_monitor:set_destroyed_cb(function()
         self._log:info('stopping thinking by request of object monitor!')
         self:_destroy_object_monitor()
         self:_stop_thinking()
      end)
   return object_monitor
end


function ExecutionUnitV2:_destroy_object_monitor()
   if self._object_monitor then
      self._log:detail('destroying object monitor')
      self._object_monitor:destroy()
      self._object_monitor = nil
   end
end

return ExecutionUnitV2
