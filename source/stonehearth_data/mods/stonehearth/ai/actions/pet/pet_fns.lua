local PetFns = class()

-- currently citizens, but could be other pets too!
function PetFns.get_friends_nearby(pet, range)
   local friends = {}
   local num_friends = 0
   local range_squared = range * range
   local pet_location = pet:add_component('mob'):get_world_location()
   local faction_name = pet:add_component('unit_info'):get_faction()
   local faction = stonehearth.population:get_faction(faction_name)

   if faction then
      local citizens = faction:get_citizens()

      for _, citizen in pairs (citizens) do
         local citizen_location = citizen:add_component('mob'):get_world_location()
         local distance_squared = (pet_location - citizen_location):distance_squared()

         if distance_squared <= range_squared then
            table.insert(friends, citizen)
            num_friends = num_friends + 1
         end
      end
   end

   return friends, num_friends
end

return PetFns
