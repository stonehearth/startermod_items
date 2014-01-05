local ExecutionUnitV2 = class()
local log = radiant.log.create_logger('ai.component')

local IDLE = 'IDLE'
local PROCESSING = 'PROCESSING'
local READY = 'READY'
local STARTED = 'STARTED'
local RUNNING = 'RUNNING'
local DEAD = 'DEAD'

function ExecutionUnitV2:__init(ai_component, entity, injecting_entity)
   self._entity = entity
   self._ai_component = ai_component
   self._nested_frames = {}
end

function ExecutionUnitV2:_destroy()
   -- wheeeeeeeeeeee....  get rid of everything!
   assert(false)
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

function ExecutionUnitV2:get_run_args()
   return self._run_args
end

function ExecutionUnitV2:is_runnable(frame)
   assert(self._frame == frame)
   return self._action.priority > 0 and self._run_args ~= nil and self._state == READY
end

function ExecutionUnitV2:join_execution_frame(frame, args)
   assert(not self._frame)
   assert(self._state == nil)
   self:_verify_execution_args(args)
   self._frame = frame
   self._args = args
   self._state = IDLE
   self._run_args = nil
end

function ExecutionUnitV2:leave_execution_frame(frame)
   assert(self._frame == frame)
   self._frame = nil
   self._args = nil
   self._run_args = nil
   self._state = DEAD
   self:stop_frame()
end

function ExecutionUnitV2:start_background_processing(frame)
   assert(self._frame == frame)
   assert(self._state == PROCESSING or self._state == IDLE)
   if self._state == IDLE then
      -- don't start processing if we already have run args...
      if not self._run_args then      
         self._state = PROCESSING
         if self._action.start_background_processing then
            self._action:start_background_processing(self, self._entity, unpack(self._args))
         else
            self:complete_background_processing(unpack(self._args))
         end
      end
   end
end

function ExecutionUnitV2:stop_background_processing(frame)
   assert(self._frame == frame)
   assert(self._state == PROCESSING or self._state == READY)
   if self._state == PROCESSING or self._state == READY then
      if self._action.stop_background_processing then
         self._action:stop_background_processing(self, self._entity)
      end
      if self._state == PROCESSING then
         self._state = IDLE
      end
   end
end

function ExecutionUnitV2:complete_background_processing(...)
   assert(self._frame)
   assert(self._state == PROCESSING)
   
   self._state = READY
   self._run_args = { ... }
   if self._frame then
      self._frame:on_unit_state_change(self)
   end   
end

function ExecutionUnitV2:on_background_processing_complete(...)
   assert(self._frame)
   assert(self._state == PROCESSING)
   self._run_args = { ... }
   if self._frame then
      self._frame:on_unit_state_change(self)
   end   
end

function ExecutionUnitV2:start_frame(frame)
   assert(self:is_runnable(frame))
   assert(self._state == READY)
   
   if self._action.start then
      self._action:start()
   end
   self._state = STARTED
end

function ExecutionUnitV2:run_frame(frame)
   assert(self._run_args)
   assert(self:get_priority() > 0)
   assert(self._state == STARTED)
   
   log:debug('%s coroutine starting action: %s (%s)',
             self._entity, self:get_name(), self._frame:format_args(self._args))
   
   self._state = RUNNING
   local result = { self._action:run(self, self._entity, unpack(self._run_args)) }
   
   log:debug('%s coroutine finished: %s', self._entity, tostring(self:get_name()))
   return result
end

function ExecutionUnitV2:stop_frame()
   if self._state == RUNNING then
      if self._action.stop then
         self._action:stop()
      end
      self._run_args = nil
      self._state = IDLE
   end
end

function ExecutionUnitV2:_verify_execution_args(args)
   if #args ~= #self._action.args then
      self:abort('unexpected number of arguments passed to %s (expected:%d got:%d)',
                  self:get_name(), #self._action.args, #args)
   end
   for i, arg in ipairs(args) do
      local expected = self._action.args[i]
      if not self:_is_type(arg, expected) then
         self:abort('unexpected argument %d in %s (expected:%s got %s)', self:get_name(),
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

-- Action facing functions...
function ExecutionUnitV2:set_action_priority(action, priority)
   self:set_action(action)
   if priority ~= self._action.priority then
      self._action.priority = priority
      -- notify the frame if we're currently in one.  we might not be! legacy
      -- actions act crazily sometimes...
      if self._frame then
         self._frame:on_unit_state_change(self)
      end
   end
end

function ExecutionUnitV2:execute(...)
   assert(self._frame)
   assert(not self._chained_execution_frame)
   self._chained_execution_frame = self._ai_component:spawn(...)
   self._chained_execution_frame:run()
   self._chained_execution_frame:_destroy()
   self._chained_execution_frame = nil
end 

function ExecutionUnitV2:spawn(...)
   local frame = self._ai_component:spawn(...)
   table.insert(self._nested_frames, frame)
   return frame
end

function ExecutionUnitV2:suspend()
   self._ai_component:suspend_thread()
end

function ExecutionUnitV2:resume()
   self._ai_component:resume_thread()
end

function ExecutionUnitV2:abort(reason)
   assert(self._frame)
   self._ai_component:abort(reason)
end

return ExecutionUnitV2
