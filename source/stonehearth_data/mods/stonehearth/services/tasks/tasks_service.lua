local TaskScheduler = require 'services.tasks.task_scheduler'
local TasksService = class()

function TasksService:__init()
   self._schedulers = {}
end

--[[
function TasksService:join_scheduler(entity, name)
   self:_get_scheduler(name):join(entity)
end

function TasksService:leave_scheduler(entity, name)
   self:_get_scheduler(name):leave(entity)
end

function TasksService:set_scheduler_activity(name, ...)
   self:_get_scheduler(name):set_activity({...})
end

function TasksService:create_task(name)
   return self:_get_scheduler(name):create_task()
end
]]
function TasksService:get_scheduler(name)
   -- todo: break out by faction, too!
   local scheduler = self._schedulers[name]
   if not scheduler then
      scheduler = TaskScheduler(name)
      self._schedulers[name] = scheduler
   end
   return scheduler
end

return TasksService()
