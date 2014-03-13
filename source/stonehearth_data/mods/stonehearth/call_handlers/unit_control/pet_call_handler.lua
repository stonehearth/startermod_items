local PetCallHandler = class()

function PetCallHandler:release_pet(session, response, critter)
   local equipment = critter:get_component('stonehearth:equipment')
   if equipment then
      local outfit = equipment:unequip_item('stonehearth:pet_collar')
      if outfit then
         radiant.entities.destroy_entity(outfit)
      end
   end

   critter:get_component('unit_info'):set_faction('critter')
   --Remove the unit info
   -- leave the original description. we'll have another mechanism for this
   --critter:get_component('unit_info'):set_description("Unwanted, but free.")
end

return PetCallHandler