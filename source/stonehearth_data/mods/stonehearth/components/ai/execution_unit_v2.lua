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
   self._processing = false
   self._id = NEXT_UNIT_ID
   NEXT_UNIT_ID = NEXT_UNIT_ID + 1
   
   self._log = radiant.log.create_logger('ai.exec_unit')
   
   self._ai_interface = { ___execution_unit = self }   
   local chain_function = function (fn)
      self._ai_interface[fn] = function(i, ...) return self['__'..fn](self, ...) end
   end
   chain_function('set_run_arguments')
   chain_function('revoke_run_arguments')
   chain_function('set_priority')
   chain_function('spawn')
   chain_function('execute')   
   chain_function('suspend')
   chain_function('resume')
   chain_function('abort')
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

function ExecutionUnitV2:__set_run_arguments(...)
   assert(self._state == PROCESSING)
   self._run_args = { ... }
   self:_set_state(READY)
end

function ExecutionUnitV2:__revoke_run_arguments(...)
   if self._state == READY then
      self._run_args = nil
      self:_set_state(PROCESSING)
   elseif self._state == RUNNING then
      self:__abort('action is no longer ready to run')
   else
      assert(not self._run_args, string.format('unexpected stated %s in revoke_run_arguments', self._state))
   end
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

function ExecutionUnitV2:__execute(...)
   self._log:spam('execute %s called', select(1, ...))
   assert(not self._execute_frame)
   self._execute_frame = self._ai_component:spawn_debug_route(self._debug_route, ...)
   self._execute_frame:run()
   
   -- execute_frame can be nil here if we got terminated early, usually because
   -- the action called ai:abort().
   if self._execute_frame then
      self._execute_frame:stop()
      self._execute_frame:destroy()
      self._execute_frame = nil
   end
end 

function ExecutionUnitV2:__spawn(...)
   self._log:spam('spawn %s called', select(1, ...))
   return self._ai_component:spawn_debug_route(self._debug_route, ...)
end

function ExecutionUnitV2:__suspend()
   self._log:spam('suspend called')
   self._ai_component:suspend_thread()
end

function ExecutionUnitV2:__resume()
   self._log:spam('resume called')
   self._ai_component:resume_thread()
end

function ExecutionUnitV2:__abort(reason)
   self._log:spam('abort %s called', tostring(reason))
   self:stop_thinking()
   self:stop()
   --assert(false, "i am way too sleepy to implement abort")
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
function ExecutionUnitV2:get_activity()
   return self._action.does
end

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

function ExecutionUnitV2:get_run_args()
   return self._run_args
end

function ExecutionUnitV2:is_runnable()
   return self._action.priority > 0 and self._run_args ~= nil and self._state == READY
end

function ExecutionUnitV2:is_active()
   return self:is_runnable() or self._state == STARTED or self._state == RUNNING or self._state == HALTED
end

function ExecutionUnitV2:initialize(args)
   self._log:spam('initialize')
   self:_verify_execution_args(args)
   self._args = args
   self._run_args = nil
   if self._state == IDLE then
      assert(not self._execute_frame)
   else
      self:_set_state(IDLE)
   end
end

function ExecutionUnitV2:start_thinking()
   self._log:spam('start_thinking (statestart_background_processing (state:%s processing:%s)', self._state, tostring(self._processing))
   
   if not self._processing then
      self._processing = true
      if not self._run_args then
         self:_set_state(PROCESSING)
         if self._action.start_thinking then
            self._action:start_thinking(self._ai_interface, self._entity, unpack(self._args))
         else
            self._log:spam('action does not implement start_thinking')
            self:__set_run_arguments(unpack(self._args))
         end
      else
         self._log:spam('ignoring start_thinking call.  already have args!')
      end
   else
      self._log:spam('ignoring start_thinking call.  already thinking...')
   end
end

function ExecutionUnitV2:stop_thinking()
   self._log:spam('stop_thinking (state:%s processing:%s)', tostring(self._state), tostring(self._processing))
   
   if self._processing then
      self._processing = false
      if self._action.stop_thinking then
         self._action:stop_thinking(self._ai_interface, self._entity)
      else
         self._log:spam('action does not implement stop_thinking')      
      end
      if self._state == PROCESSING then
         self:_set_state(IDLE)
      end
   else
      self._log:spam('ignoring stop_thinking call.  not thinking...')
   end
end

function ExecutionUnitV2:start()
   self._log:spam('start')
   assert(self:is_runnable())
   assert(self._state == READY)
   
   if self._action.start then
      self._action:start(self._ai_interface, self._entity)
   else
      self._log:spam('action does not implement start')         
   end
   self:_set_state(STARTED)
end

function ExecutionUnitV2:run()
   self._log:spam('run')
   assert(self._run_args)
   assert(self:get_priority() > 0)
   assert(self._state == STARTED)
   
   self:_set_state(RUNNING)
   local result = nil
   if self._action.run then
      self._log:debug('%s coroutine starting action: %s (%s)',
                      self._entity, self:get_name(), stonehearth.ai:format_args(self._args))
      result = { self._action:run(self._ai_interface, self._entity, unpack(self._run_args)) }      
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
         self._action:stop(self._ai_interface, self._entity)
      else
         self._log:spam('action does not implement stop')
      end
      self._run_args = nil
      self:_set_state(IDLE)
   end
end

function ExecutionUnitV2:_verify_execution_args(args)
   if #args ~= #self._action.args then
      self:__abort('unexpected number of arguments passed to %s (expected:%d got:%d)',
                    self:get_name(), #self._action.args, #args)
   end
   for i, arg in ipairs(args) do
      local expected = self._action.args[i]
      if not self:_is_type(arg, expected) then
         self:__abort('unexpected argument %d in %s (expected:%s got %s)', self:get_name(),
                       i, tostring(expected), tostring(type(arg)))
      end      
   end
end

function ExecutionUnitV2:_is_type(var, cls)
   local t = type(var)
   if t == 'userdata' then
      return var:get_type_id() == cls.get_type_id()
   end
   return t == cls
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

function ExecutionUnitV2:_set_state(state)
   self._log:spam('state change %s -> %s', tostring(self._state), state)
   assert(state and self._state ~= state)
   
   self._state = state
   radiant.events.trigger(self, self._state, self)
   radiant.events.trigger(self, 'state_changed', self, self._state)
end

return ExecutionUnitV2
