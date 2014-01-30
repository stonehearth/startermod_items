local RunTaskAction = class()

RunTaskAction.version = 2

function RunTaskAction:__init(task)
   self._id = stonehearth.ai:get_next_object_id()   
   self._task = task
end

function RunTaskAction:_create_execution_frame(ai)
   if not self._execution_frame then
      self._ai = ai      
      local activity = self._task:get_activity()
      self._args = activity.args

      self._execution_frame = ai:spawn(activity.name, activity.args)
      radiant.events.listen(self._task, 'started', self, self._start_stop_thinking)
      radiant.events.listen(self._task, 'stopped', self, self._start_stop_thinking)
      radiant.events.listen(self._task, 'work_available', self, self._start_stop_thinking)
   end
end

function RunTaskAction:_start_stop_thinking()
   if not self._starting then
      local should_think = self._should_think and self._task:__action_can_start(self)
      if should_think and not self._thinking then
         self._thinking = true
         local think_output = self._execution_frame:start_thinking(self._ai.CURRENT)
         if think_output then
            self._ai:set_think_output(think_output)
         else
            self._execution_frame:on_ready(function(frame, think_output)
               self._ai:set_think_output()
            end)
         end

      elseif not should_think and self._thinking then      
         self._thinking = false
         self._execution_frame:stop_thinking()
      end
   end
end

function RunTaskAction:get_debug_info(debug_route)
   local info = {
      id = self._id,
      name = self.name,
      args = stonehearth.ai:format_args(self._args),
      does = self.does,
      priority = self.priority
   }
   
   if self._execution_frame then
      info.execution_frames = { self._execution_frame:get_debug_info() }
   end
   
   return info
end

function RunTaskAction:start_thinking(ai, entity)
   self:_create_execution_frame(ai)

   self._should_think = true
   self:_start_stop_thinking()
end

function RunTaskAction:stop_thinking(ai, entity)
   self._should_think = false
   self:_start_stop_thinking()
end

function RunTaskAction:start(ai)
   self._starting = true
   if not self._task:__action_try_start(self) then
      ai:abort('task would not allow us to start (max running reached?)')
      return
   end
   self._starting = false
   self._execution_frame:start()
end

function RunTaskAction:run(...)
   self._execution_frame:run(...)
   self._task:__action_completed(self)
end

function RunTaskAction:stop()
   if self._execution_frame then
      self._execution_frame:stop()
      self._execution_frame:destroy()
      self._execution_frame = nil
   end
   self._task:__action_stopped(self)
end

function RunTaskAction:destroy()
   if self._execution_frame then
      self._execution_frame:destroy()
      self._execution_frame = nil
   end
end

function RunTaskAction:_frame_not_ready(frame)
   assert(frame)
   if self._execution_frame == frame then
      self._log:debug('current frame became unready')
      self._ai:clear_think_output()
   end
end

return RunTaskAction
