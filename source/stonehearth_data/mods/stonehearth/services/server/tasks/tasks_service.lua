local Task = require 'services.server.tasks.task'
local TaskScheduler = require 'services.server.tasks.task_scheduler'

local TasksService = class()

function TasksService:__init()
   self._schedulers = {}
end

function TasksService:initialize()
   -- the tasks service doesn't save or restore tasks.  we leave
   -- that up to the owners of the tasks themselves
end

function TasksService:create_scheduler(name)
   local scheduler = self._schedulers[name]
   if not scheduler then
      scheduler = TaskScheduler(name)
      self._schedulers[name] = scheduler
   end
   return scheduler
end

return TasksService

