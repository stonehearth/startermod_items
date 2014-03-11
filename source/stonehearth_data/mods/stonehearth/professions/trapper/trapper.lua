local trapper_class = {}

function trapper_class.promote(entity)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:worker:profession_description')
   local profession_component = entity:add_component('stonehearth:profession', profession_description.profession)

   --Slap a new outfit on the crafter
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:trapper:outfit')

   --Add an inventory
   entity:add_component('stonehearth:inventory')
end

function trapper_class.demote(entity)
   local equipment = entity:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:worker_outfit')
   
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end

  -- remove the inventory
  entity:remove_component('stonehearth:inventory')
  
end

return trapper_class
