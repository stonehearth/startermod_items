local PetCallHandler = class()

function PetCallHandler:release_from_captivity(session, response, critter)
   local equipment = critter:add_component('stonehearth:equipment')
   local outfit = equipment:unequip_item('stonehearth:pet_collar')
   if outfit then
      radiant.entities.destroy_entity(outfit)
   end

   --Remove the unit info
    critter:get_component('unit_info'):set_description("Unwanted, but free.")
end

return PetCallHandler