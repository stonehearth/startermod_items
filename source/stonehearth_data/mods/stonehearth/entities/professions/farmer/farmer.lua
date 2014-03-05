local farmer_class = {}

function farmer_class.promote(entity)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:farmer:profession_description')
   local profession_component = entity:add_component('stonehearth:profession')
   profession_component:set_info(profession_description.profession)

   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')

   --TODO: change this into a farmer outfit
   equipment:equip_item('stonehearth:farmer:outfit')
end

function farmer_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end
end

return farmer_class
