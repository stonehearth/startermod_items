local RunTaskAction = require 'services.tasks.run_task_action'
local RunTaskActionFactory = class()

function RunTaskActionFactory:__init(task, priority, scheduler_activity)
   self._task = task
   self._scheduler_activity = scheduler_activity

   self.version = 2
   self.name = task:get_name()
   self.does = scheduler_activity[1]
   self.args = { select(2, unpack(scheduler_activity)) }
   self.priority = priority
end

function RunTaskActionFactory:create_action(ai_component, entity, injecting_entity)
   local action = RunTaskAction(ai_component, self._task, self._scheduler_activity)
   action.name = self.name
   action.does = self.does
   action.args = self.args
   action.priority = self.priority
   action.version = self.version
   return action
end

return RunTaskActionFactory
