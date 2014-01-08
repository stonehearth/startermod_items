local RunTaskAction = class()

function RunTaskAction:__init(ai, task, scheduler_activity)
   self._ai = ai
   self._task = task
   
   local activity = task:get_activity()
   self._execution_frame = ai:spawn(activity.name, activity.args)
   radiant.events.listen(self._execution_frame, 'ready', function()
         ai:set_think_output()
      end)

   radiant.events.listen(task, 'started', self, self._on_task_started)
   radiant.events.listen(task, 'stopped', self, self._on_task_stopped)   
   if task:get_state() == 'started' then
      self:_on_task_started()
   end
end

function RunTaskAction:set_debug_route(debug_route)
   self._execution_frame:set_debug_route(debug_route .. ' task')
end

function RunTaskAction:_on_task_started()
   if self._should_think then
      self._execution_frame:start_thinking(self._task:get_args())
   end
end

function RunTaskAction:_on_task_stopped()
   if self._should_think then
      self._execution_frame:stop_thinking()
   end
end

function RunTaskAction:start_thinking(ai, entity)
   self._should_think = true
   if self._task:get_state() == 'started' then
      self._execution_frame:start_thinking(self._task:get_args())
   end
end

function RunTaskAction:stop_thinking(ai, entity)
   self._execution_frame:stop_thinking(ai, entity)
   self._should_think = false
end

function RunTaskAction:start()
   self._execution_frame:start()
end

function RunTaskAction:run(...)
   self._execution_frame:run(...)
   self._task:__completed()
end

function RunTaskAction:stop()
   self._execution_frame:stop()
end

function RunTaskAction:destroy()
   self._execution_frame:destroy()
end

return RunTaskAction
