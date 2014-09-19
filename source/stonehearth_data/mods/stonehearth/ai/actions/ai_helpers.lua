local Point3 = _radiant.csg.Point3

local AiHelpers = class()

function AiHelpers.bump_entity(entity, vector)
   local start_location = radiant.entities.get_world_location(entity)
   local proposed_location = start_location + vector
   local proposed_grid_location = proposed_location:to_closest_int()

   local actual_location
   local resolved_grid_location = radiant.terrain.get_direct_path_end_point(start_location, proposed_location, entity, true)

   local clear_path = resolved_grid_location == proposed_grid_location

   if clear_path then
      -- move the to floating point location
      actual_location = proposed_location
   else
      -- move as far as we could go
      actual_location = resolved_grid_location
   end

   radiant.entities.move_to(entity, actual_location)
end

function AiHelpers.calculate_bump_vector(bumper, bumpee, separation)
   local bumper_location = radiant.entities.get_world_location(bumper)
   local bumpee_location = radiant.entities.get_world_location(bumpee)
   local vector = bumpee_location - bumper_location
   vector.y = 0   -- only x,z bumping permitted

   local initial_distance = vector:length()
   local bump_distance = separation - initial_distance

   if bump_distance <= 0 then
      return Point3.zero
   end

   if initial_distance ~= 0 then
      vector:normalize()
   else
      vector = radiant.math.random_xz_unit_vector()
   end

   vector:scale(bump_distance)

   return vector
end

return AiHelpers
