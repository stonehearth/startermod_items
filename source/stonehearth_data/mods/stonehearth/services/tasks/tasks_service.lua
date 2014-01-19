local TaskScheduler = require 'services.tasks.task_scheduler'
local TasksService = class()

function TasksService:__init()
   self._named_schedulers = {}
   self._unnamed_schedulers = {}
   self._compound_task_ctors = {}
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

function TasksService:register_compound_task(name, ctor)
   assert(not self._compound_task_ctors[name])
   self._compound_task_ctors[name] = ctor
end

function TasksService:get_compound_task(name)
   return self._compound_task_ctors[name]
end

return TasksService()
