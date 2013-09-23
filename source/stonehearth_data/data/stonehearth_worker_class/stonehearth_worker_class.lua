local WorkerScheduler = require 'lib.worker_scheduler'
local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local stonehearth_worker_class = {}

function stonehearth_worker_class.promote(entity)
   radiant.entities.xxx_inject_into_entity(entity, 'stonehearth_worker_class', 'class_info')

   -- xxx: this is strictly temporary.  the code will be factored into stonehearth_classes soon.
   local outfit = radiant.entities.create_entity('stonehearth_worker_class', 'worker_outfit')
   local render_info = entity:add_component('render_info')
   render_info:attach_entity(outfit)
   -- end xxx:
end

function stonehearth_worker_class.demote(entity)
   radiant.entities.xxx_unregister_from_entity(entity, 'stonehearth_worker_class', 'class_info')

   -- xxx: this is strictly temporary.  the code will be factored into stonehearth_classes soon.
   local render_info = entity:add_component('render_info')
   local outfit = render_info:remove_entity('stonehearth_worker_class', 'worker_outfit')
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
   -- end xxx:

   --The worker doesn't have to drop his axe on demote since all people can be demoted
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
