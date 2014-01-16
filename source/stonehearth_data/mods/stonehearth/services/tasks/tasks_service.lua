local TaskScheduler = require 'services.tasks.task_scheduler'
local TasksService = class()

function TasksService:__init()
   self._schedulers = {}
end

function TasksService:get_scheduler(name, faction)
   -- todo: break out by faction, too!
   local scheduler = self._schedulers[name]
   if not scheduler then
      scheduler = TaskScheduler(name)
      self._schedulers[name] = scheduler
   end
   return scheduler
end

return TasksService()
