local carpenter_class = {}

function carpenter_class.promote(entity, workshop_component)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:carpenter:profession_description')
   local profession_component = entity:add_component('stonehearth:profession')
   profession_component:set_info(profession_description.profession)

   local crafter_component = entity:add_component("stonehearth:crafter")
   crafter_component:set_info(profession_description.crafter)

   crafter_component:set_workshop(workshop_component)
   workshop_component:set_crafter(entity)

   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:carpenter:outfit')
end

function carpenter_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end

  --move saw back to the bench
  --local crafter_component = entity:get_component("stonehearth:crafter")
  --local bench_component = crafter_component:get_workshop()
end

return carpenter_class
