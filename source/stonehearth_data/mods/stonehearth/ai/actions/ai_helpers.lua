local Point3f = _radiant.csg.Point3f

local AiHelpers = class()

function AiHelpers.calculate_bump_vector(bumper, bumpee, desired_distance)
   local bumper_location = radiant.entities.get_world_location(bumper)
   local bumpee_location = radiant.entities.get_world_location(bumpee)
   local vector = bumpee_location - bumper_location
   vector.y = 0   -- only x,z bumping permitted

   local initial_distance = vector:length()
   local bump_distance = desired_distance - initial_distance

   if bump_distance <= 0 then
      return Point3f.zero
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
