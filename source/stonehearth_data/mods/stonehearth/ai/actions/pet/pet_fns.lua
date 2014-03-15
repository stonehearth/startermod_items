local PetFns = class()

-- currently citizens, but could be other pets too!
function PetFns.get_friends_nearby(pet, range)
   local friends = {}
   local num_friends = 0
   local range_squared = range * range
   local pet_location = pet:add_component('mob'):get_world_location()
   local faction = pet:add_component('unit_info'):get_faction()

   local friendly_pops = stonehearth.population:get_friendly_populations(faction)
   for _, pop in pairs(friendly_pops) do
      local citizens = pop:get_citizens()
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
