local trapper_class = {}

function trapper_class.promote(entity)

   --Add the necessary components
   local profession_description = radiant.resources.load_json('stonehearth:trapper:description')
   local profession_component = entity:add_component('stonehearth:profession')
   profession_component:set_info(profession_description.profession)

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
  
  --move saw back to the bench
  --local crafter_component = entity:get_component("stonehearth:crafter")
  --local bench_component = crafter_component:get_workshop()
end

return trapper_class
