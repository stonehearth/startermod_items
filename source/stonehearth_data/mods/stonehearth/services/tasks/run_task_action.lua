local RunTaskAction = class()

function RunTaskAction:__init(ai, task, scheduler_activity)
   self._id = stonehearth.ai:get_next_object_id()   
   self._ai = ai
   self._task = task
   
   local activity = task:get_activity()
   self._execution_frame = ai:spawn(activity.name, activity.args)
   radiant.events.listen(self._execution_frame, 'ready', function()
         ai:set_think_output()
      end)
   self._args = activity.args

   radiant.events.listen(task, 'started', self, self._start_stop_thinking)
   radiant.events.listen(task, 'stopped', self, self._start_stop_thinking)
   radiant.events.listen(task, 'work_available', self, self._start_stop_thinking)
end

function RunTaskAction:_start_stop_thinking()
   if not self._starting then
      local should_think = self._should_think and self._task:__action_can_start(self)
      if should_think and not self._thinking then
         self._thinking = true
         self._execution_frame:capture_entity_state()
         self._execution_frame:start_thinking()
      elseif not should_think and self._thinking then      
         self._thinking = false
         self._execution_frame:stop_thinking()
      end
   end
end

function RunTaskAction:get_debug_info(debug_route)
   return {
      id = self._id,
      name = self.name,
      args = stonehearth.ai:format_args(self._args),
      does = self.does,
      priority = self.priority,
      execution_frames = {
         self._execution_frame:get_debug_info()
      }
   }
end

function RunTaskAction:set_debug_route(debug_route)
   self._execution_frame:set_debug_route(debug_route .. ' ' .. self._task:get_name())
end

function RunTaskAction:start_thinking(ai, entity)
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
   self._execution_frame:stop()
   self._task:__action_stopped(self)
end

function RunTaskAction:destroy()
   self._execution_frame:destroy()
end

return RunTaskAction
