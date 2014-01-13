local ExecutionUnitV2 = class()

local IDLE = 'idle'
local PROCESSING = 'processing'
local READY = 'ready'
local STARTED = 'started'
local RUNNING = 'running'
local HALTED = 'halted'
local DEAD = 'dead'

local NEXT_UNIT_ID = 1

--[[

         idle
           |
         (call action:start_thinking)
           |
         processing
           |
         (action calls ai:complete_processing)
           |
         ready
           |
         (call action:start)
           |
         (call action:run)
           |
         running
           |
         (action run returns)
           |
         (call action:stop)
           |
           +
         (return to idle...)         
]]

function ExecutionUnitV2:__init(ai_component, entity, injecting_entity)
   -- sometimes (ef -> compound_action -> ef) we get an ai_component that's actually the interface.
   -- if so, reach through the matrix and grab the real component
   if ai_component.___execution_unit then
      ai_component = ai_component.___execution_unit._ai_component
   end
   assert(ai_component._entity)
   
   self._entity = entity
   self._ai_component = ai_component
   self._execute_frame = nil
   self._thinking = false
   self._state = IDLE
   self._id = NEXT_UNIT_ID
   NEXT_UNIT_ID = NEXT_UNIT_ID + 1
   
   self._log = radiant.log.create_logger('ai.exec_unit')
   
   self._ai_interface = { ___execution_unit = self }   
   local chain_function = function (fn)
      self._ai_interface[fn] = function(i, ...) return self['__'..fn](self, ...) end
   end
   chain_function('set_think_output')
   chain_function('clear_think_output')
   chain_function('set_priority')
   chain_function('spawn')
   chain_function('execute')   
   chain_function('suspend')
   chain_function('resume')
   chain_function('abort')
   chain_function('get_log')
end

function ExecutionUnitV2:get_state()
   return self._state
end

function ExecutionUnitV2:set_debug_route(debug_route)
   self._debug_route = debug_route .. ' u:' .. tostring(self._id)
   local prefix = string.format('%s (%s)', self._debug_route, self:get_name())
   self._log:set_prefix(prefix)
   
   -- for compoundaction...
   if self._action.set_debug_route then
      self._action:set_debug_route(self._debug_route)
   end
end

function ExecutionUnitV2:get_action_interface()
   return self._ai_interface
end

function ExecutionUnitV2:__set_think_output(think_output)
   self._log:spam('set_think_output: %s', stonehearth.ai:format_args(think_output))
   assert(think_output == nil or type(think_output) == 'table')
   assert(self._state == PROCESSING)
   if not think_output then
      think_output = self._args
   end
   self:_verify_arguments(think_output, self._action.think_output or self._action.args)
   self._think_output = think_output
   self:_set_state(READY)
end

function ExecutionUnitV2:__clear_think_output(...)
   if self._state == READY then
      self._think_output = nil
      self:_set_state(PROCESSING)
   elseif self._state == RUNNING then
      self:__abort('action is no longer ready to run')
   else
      assert(not self._think_output, string.format('unexpected stated %s in clear_think_output', self._state))
   end
end

function ExecutionUnitV2:__get_log()
   return self._log
end

function ExecutionUnitV2:__set_priority(action, priority)
   if not self._action then
      self:set_action(action)
   end
   if priority ~= self._action.priority then
      self._action.priority = priority
      -- notify the frame if we're currently in one.  we might not be! legacy
      -- actions act crazily sometimes...
      radiant.events.trigger(self, 'priority_changed', self, priority)
   end
end

function ExecutionUnitV2:__execute(name, args)
   self._log:spam('execute %s %s called', name, stonehearth.ai:format_args(args))
   assert(not self._execute_frame)
   self._execute_frame = self._ai_component:spawn_debug_route(self._debug_route, name, args)
   self._execute_frame:set_current_entity_state(self._current_entity_state)
   self._execute_frame:run()
   
   -- execute_frame can be nil here if we got terminated early, usually because
   -- the action called ai:abort().
   if self._execute_frame then
      self._execute_frame:stop()
      self._execute_frame:destroy()
      self._execute_frame = nil
   end
end 

function ExecutionUnitV2:__spawn(name, args)   
   self._log:spam('spawn %s called', name)
   return self._ai_component:spawn_debug_route(self._debug_route, name, args)
end

function ExecutionUnitV2:__suspend(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('suspend called (reason: %s)', reason)
   self._ai_component:suspend_thread()
end

function ExecutionUnitV2:__resume(format, ...)
   local reason = format and string.format(format, ...) or 'no reason given'
   self._log:spam('resume called (reason: %s)', reason)
   self._ai_component:resume_thread()
end

function ExecutionUnitV2:__abort(reason, ...)
   local reason = string.format(reason, ...)
   self._log:spam('abort %s called', tostring(reason))
   self:stop_thinking()
   self:stop()
   -- why not destroy? -- tony
   self._ai_component:_terminate_thread()
end

function ExecutionUnitV2:spawn(...)
   return self:__spawn(...)
end

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
   self._action = nil
   self:_set_state(DEAD)
   radiant.events.unpublish(self)   
end

-- AiService facing functions...
function ExecutionUnitV2:set_action(action)
   self._action = action
   assert(action.name)
   assert(action.does)
   assert(action.args)
   assert(action.priority)
end

function ExecutionUnitV2:get_action()
   return self._action
end

-- ExecutionFrame facing functions...
function ExecutionUnitV2:get_name()
   return self._action.name
end

function ExecutionUnitV2:get_version()
   return self._action.version
end

function ExecutionUnitV2:get_priority()
   return self._action.priority
end

function ExecutionUnitV2:get_weight()
   return self._action.weight or 1
end

function ExecutionUnitV2:get_think_output()
   return self._think_output
end

function ExecutionUnitV2:is_runnable()
   return self._action.priority > 0 and self._think_output ~= nil and self._state == READY
end

function ExecutionUnitV2:is_active()
   return self:is_runnable() or self._state == STARTED or self._state == RUNNING or self._state == HALTED
end

function ExecutionUnitV2:initialize(args)
   self._log:spam('initialize')

   if self:_verify_arguments(args, self._action.args) then
      self._think_output = nil
      if self._state == IDLE then
         assert(not self._execute_frame)
      else
         self:_set_state(IDLE)
      end
   end
   if self._action.eval_arguments then
      self._log:spam('eval_arguments...')
      self._args = self._action:eval_arguments(self, self._entity, args)
      self._log:spam('eval_arguments returned %s', stonehearth.ai:format_args(self._args))
   else
      self._args = args
   end   
   return self._args
end

-- used by the compound action...
function ExecutionUnitV2:short_circuit_thinking()
   self._log:spam('short_circuit_thinking')
   assert(not self._thinking)
   
   self._log:spam('short-circuiting think by request.  going right to ready!')
   if self._state ~= READY then
      self:_set_state(READY)
   end
   self._think_output = {}
end

function ExecutionUnitV2:start_thinking(current_entity_state)
   self._log:spam('start_thinking (state:%s processing:%s)', self._state, tostring(self._thinking))

   if self._thinking then
      self._log:spam('call to start_thinking (%s) while already thinking with (%s).  stopping...',
                     tostring(current_entity_state.location), tostring(self._current_entity_state.location))
      self:stop_thinking()
      self._log:spam('resuming start_thinking, now that we stopped.')
   end
   
   assert(current_entity_state)
   assert(not self._thinking)
   self._log:spam('CURRENT.location predicted to be %s', current_entity_state.location)
   
   self._thinking = true
   self._current_entity_state = current_entity_state
   self._ai_interface.CURRENT = current_entity_state
   if not self._think_output then
      self:_set_state(PROCESSING)
      if self._action.start_thinking then
         self._action:start_thinking(self._ai_interface, self._entity, self._args)
      else
         self._log:spam('action does not implement start_thinking')
         self:__set_think_output(self._args)
      end
   else
      self._log:spam('ignoring start_thinking call.  already have args!')
   end
end

function ExecutionUnitV2:stop_thinking()
   self._log:spam('stop_thinking (state:%s processing:%s)', tostring(self._state), tostring(self._thinking))
   
   if self._thinking then
      self._thinking = false
      self._think_output = nil
      if self._action.stop_thinking then
         self._action:stop_thinking(self._ai_interface, self._entity)
      else
         self._log:spam('action does not implement stop_thinking')      
      end
      if self._state == PROCESSING then
         self:_set_state(IDLE)
      end
      self._ai_interface.CURRENT = nil
   else
      self._log:spam('ignoring stop_thinking call.  not thinking...')
   end
end

function ExecutionUnitV2:start()
   self._log:spam('start')
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
   self._log:spam('run')
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
   self:_set_state(HALTED)
   return result
end

function ExecutionUnitV2:stop()
   self._log:spam('stop')
   
   if self:_in_state(RUNNING, HALTED) then
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
   for name, expected_type in pairs(args_prototype) do
      if args[name] == nil then
         if type(expected_type) == 'table' and not radiant.util.is_instance(expected_type) then
            args[name] = expected_type.default
         end
      end
      if args[name] == nil then
         self:__abort('missing argument "%s" in "%s".', name, self:get_name())
         return false
      end
   end
end

-- XXXXXXXXXXXXXXXXX: make a State helper class!!
function ExecutionUnitV2:_in_state(...)
   for _, state in ipairs({...}) do
      if self._state == state then
         return true
      end
   end
   return false
end

function ExecutionUnitV2:get_current_entity_state()
   assert(self._current_entity_state)
   return self._current_entity_state
end

function ExecutionUnitV2:_set_state(state)
   self._log:spam('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   
   self._state = state
   radiant.events.trigger(self, self._state, self)
   radiant.events.trigger(self, 'state_changed', self, self._state)
end

function ExecutionUnitV2:_get_action_debug_info()
   if self._action.get_debug_info then
      return self._action:get_debug_info(self, self._entity)
   end
   return {
      name = self._action.name,
      does = self._action.does,
      priority = self._action.priority,
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

return ExecutionUnitV2
