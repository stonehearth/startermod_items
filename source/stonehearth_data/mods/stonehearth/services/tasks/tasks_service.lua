local TaskScheduler = require 'services.tasks.task_scheduler'
local TasksService = class()

function TasksService:__init()
   self._named_schedulers = {}
   self._unnamed_schedulers = {}
end

function TasksService:get_scheduler(name, faction)
   -- todo: break out by faction, too!
   local scheduler = self._named_schedulers[name]
   if not scheduler then
      scheduler = TaskScheduler(name)
      self._named_schedulers[name] = scheduler
   end
   return scheduler
end

function TasksService:create_scheduler()
   local scheduler = TaskScheduler('unnamed')
   self._unnamed_schedulers[scheduler] = true
   return scheduler
end

function TasksService:destroy_scheduler(scheduler)
   assert(self._unnamed_schedulers[scheduler])
   scheduler:destroy()
   self._unnamed_schedulers[scheduler] = nil
end

return TasksService()
