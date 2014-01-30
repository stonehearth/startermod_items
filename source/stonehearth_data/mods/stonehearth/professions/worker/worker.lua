local Point3 = _radiant.csg.Point3

local singleton = {
   _schedulers = {}
}
local worker_class = {}

function worker_class.promote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:worker_outfit')

   local profession_description = radiant.resources.load_json('stonehearth:worker:profession_description')
   local profession_component = entity:add_component('stonehearth:profession')
   profession_component:set_info(profession_description.profession)

   stonehearth.tasks:get_scheduler('stonehearth:workers', radiant.entities.get_faction(entity))
                        :join(entity)
end


function worker_class.demote(entity)
   stonehearth.tasks:get_scheduler('stonehearth:workers', radiant.entities.get_faction(entity))
                        :leave(self._entity)

   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')

   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
end

return worker_class
