local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local worker_class = {}

function worker_class.promote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth.worker_outfit')

   local job_description = radiant.resources.load_json('stonehearth.worker.job_description')
   local job_info_component = entity:add_component('stonehearth:job_info')
   job_info_component:set_info(job_description.job_info)
end


function worker_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth.worker_outfit')

   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
end

return worker_class
