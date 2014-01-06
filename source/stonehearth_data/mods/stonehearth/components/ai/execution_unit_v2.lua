local ExecutionUnitV2 = class()
local log = radiant.log.create_logger('ai.component')

local IDLE = 'idle'
local PROCESSING = 'processing'
local READY = 'ready'
local STARTED = 'started'
local RUNNING = 'runnin'
local DEAD = 'dead'

function ExecutionUnitV2:__init(ai_component, entity, injecting_entity)
   self._entity = entity
   self._ai_component = ai_component
   self._execute_frame = nil
   self._nested_frames = {}
   
   self._ai_interface = { ___execution_unit = self }   
   local chain_function = function (fn)
      self._ai_interface[fn] = function(i, ...) return self['__'..fn](self, ...) end
   end
   chain_function('complete_background_processing')
   chain_function('set_priority')
   chain_function('spawn')
   chain_function('execute')   
   chain_function('suspend')
   chain_function('resume')
   chain_function('abort')   
end

function ExecutionUnitV2:get_action_interface()
   return self._ai_interface
end

function ExecutionUnitV2:__complete_background_processing(...)
   assert(self._state == PROCESSING)   
   self._run_args = { ... }
   self:_set_state(READY)
end

function ExecutionUnitV2:__set_priority(action, priority)
   self:set_action(action)
   if priority ~= self._action.priority then
      self._action.priority = priority
      -- notify the frame if we're currently in one.  we might not be! legacy
      -- actions act crazily sometimes...
      radiant.events.trigger(self, 'priority_changed', self, priority)
   end
end

function ExecutionUnitV2:__execute(...)
   assert(not self._execute_frame)
   self._execute_frame = self._ai_component:spawn(...)
   self._execute_frame:run()
   self._execute_frame:terminate()
   self._execute_frame = nil
end 

function ExecutionUnitV2:__spawn(...)
   local frame = self._ai_component:spawn(...)
   table.insert(self._nested_frames, frame)
   return frame
end

function ExecutionUnitV2:__suspend()
   self._ai_component:suspend_thread()
end

function ExecutionUnitV2:__resume()
   self._ai_component:resume_thread()
end

function ExecutionUnitV2:__abort(reason)
   self._ai_component:abort(reason)
end

function ExecutionUnitV2:spawn(...)
   return self:__spawn(...)
end

function ExecutionUnitV2:destroy()
   if self._execute_frame then
      self._execute_frame:terminate()
   end
   for _, frame in ipairs(self._nested_frames) do
      frame:terminate()
   end
   
   if self._state == IDLE or self._state == READY or self._state == DEAD then
      -- nothing to do
   elseif self._state == PROCESSING then
      self:stop_background_processing()
   elseif self._state == RUNNING or self._state == STARTED then
      self:stop()
   else
      assert(false, 'unknown state in destroy')
   end
   if self._action.destroy then
      self._action:destroy(self._ai_interface, self._entity)
      self._action = nil
   end
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

function ExecutionUnitV2:initialize(args)
   self:_verify_execution_args(args)
   self._args = args
   self._run_args = nil
   self:_set_state(IDLE)
end

function ExecutionUnitV2:start_background_processing()
   assert(self._state == PROCESSING or self._state == IDLE)
   if self._state == IDLE then
      -- don't start processing if we already have run args...
      if not self._run_args then
         self:_set_state(PROCESSING)
         if self._action.start_background_processing then
            self._action:start_background_processing(self._ai_interface, self._entity, unpack(self._args))
         else
            self:__complete_background_processing(unpack(self._args))
         end
      end
   end
end

function ExecutionUnitV2:stop_background_processing()
   assert(self._state == PROCESSING or self._state == READY)
   if self._state == PROCESSING or self._state == READY then
      if self._action.stop_background_processing then
         self._action:stop_background_processing(self._ai_interface, self._entity)
      end
      if self._state == PROCESSING then
         self:_set_state(IDLE)
      end
   end
end

function ExecutionUnitV2:start()
   assert(self:is_runnable())
   assert(self._state == READY)
   
   if self._action.start then
      self._action:start(self._ai_interface, self._entity)
   end
   self:_set_state(STARTED)
end

function ExecutionUnitV2:run()
   assert(self._run_args)
   assert(self:get_priority() > 0)
   assert(self._state == STARTED)
   
   log:debug('%s coroutine starting action: %s (%s)',
             self._entity, self:get_name(), stonehearth.ai:format_args(self._args))

   self:_set_state(RUNNING)
   local result = { self._action:run(self._ai_interface, self._entity, unpack(self._run_args)) }
   
   log:debug('%s coroutine finished: %s', self._entity, tostring(self:get_name()))
   return result
end

function ExecutionUnitV2:stop()
   if self._state == RUNNING then
      if self._action.stop then
         self._action:stop(self._ai_interface, self._entity)
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

function ExecutionUnitV2:_set_state(state)
   assert (self._state ~= state)
   self._state = state
   radiant.events.trigger(self, self._state, { execution_unit = self })
   radiant.events.trigger(self, 'state_changed', { execution_unit = self, state = self._state })
end

return ExecutionUnitV2
