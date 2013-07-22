local WorkerScheduler = radiant.mods.require('/stonehearth_worker_class/lib/worker_scheduler.lua')

local singleton = {
   _schedulers = {}
}
local stonehearth_worker_class = {}

function stonehearth_worker_class.promote(entity)
   radiant.entities.inject_into_entity(entity, '/stonehearth_worker_class/class_info/')
end

function stonehearth_worker_class.get_worker_scheduler(faction)
   radiant.check.is_string(faction)
   local scheduler = singleton._schedulers[faction]
   if not scheduler then
      scheduler = WorkerScheduler(faction)
      singleton._schedulers[faction] = scheduler
   end
   return scheduler
end

return stonehearth_worker_class
