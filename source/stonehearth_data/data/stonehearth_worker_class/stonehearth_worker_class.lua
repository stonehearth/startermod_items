local WorkerScheduler = radiant.mods.require('/stonehearth_worker_class/lib/worker_scheduler.lua')
local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local stonehearth_worker_class = {}
local class_info_url = '/stonehearth_worker_class/class_info/'

function stonehearth_worker_class.promote(entity)
   radiant.entities.inject_into_entity(entity, class_info_url)

   -- xxx: this is strictly temporary.  the code will be factored into stonehearth_classes soon.
   local outfit = radiant.entities.create_entity('/stonehearth_worker_class/entities/worker_outfit')
   local render_info = entity:add_component('render_info')
   render_info:attach_entity(outfit)
   -- end xxx:
end

function stonehearth_worker_class.demote(entity)
   radiant.entities.unregister_from_entity(entity, class_info_url)

   -- xxx: this is strictly temporary.  the code will be factored into stonehearth_classes soon.
   local render_info = entity:add_component('render_info')
   local outfit = render_info:remove_entity('/stonehearth_worker_class/entities/worker_outfit')
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
   -- end xxx:

   --drop talisman back into the world (can be a fresh copy)
   --TODO: move the axe entity elsewhere
   local axe = radiant.entities.create_entity('/stonehearth_human_race/rigs/male/effects/worker_axe')
   local dude_loc = radiant.entities.get_location_aligned(entity)

   --TODO: where should he put it? Ideally it would pop out of him and into...?
   radiant.terrain.place_entity(axe, Point3(dude_loc.x , 6, dude_loc.z + 3))
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
