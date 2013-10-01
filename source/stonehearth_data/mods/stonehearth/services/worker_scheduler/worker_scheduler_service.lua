local WorkerScheduler = require 'services.worker_scheduler.worker_scheduler'
local WorkerSchedulerService = class()

function WorkerSchedulerService:__init()
   self._worker_schedulers =  {}
end

function WorkerSchedulerService:get_worker_scheduler(faction)
   radiant.check.is_string(faction)
   local scheduler = self._worker_schedulers[faction]
   if not scheduler then
      scheduler = WorkerScheduler(faction)
      self._worker_schedulers[faction] = scheduler
   end
   return scheduler
end

return WorkerSchedulerService()
