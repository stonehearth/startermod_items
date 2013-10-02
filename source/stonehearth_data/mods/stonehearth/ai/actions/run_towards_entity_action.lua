local RunTowardsEntity = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

RunTowardsEntity.name = 'run away from an entity'
RunTowardsEntity.does = 'stonehearth.run_towards_entity'
RunTowardsEntity.priority = 1

function RunTowardsEntity:run(ai, entity, target)
   assert(target)

   local my_location = radiant.entities.get_world_grid_location(entity)
   local target_location = radiant.entities.get_world_grid_location(target)

   local run_vector = Vec3(target_location.x - my_location.x, my_location.y, target_location.z - my_location.z)
   run_vector:normalize()

   run_vector.x = self:round(run_vector.x) * 3
   run_vector.z = self:round(run_vector.z) * 3

   local goto_location = Point3(my_location.x + run_vector.x, my_location.y, my_location.z + run_vector.z)

   ai:execute('stonehearth.goto_location', goto_location)
end

function RunTowardsEntity:round(num, idp)
  local mult = 10^(idp or 0)
  return math.floor(num * mult + 0.5) / mult
end

return RunTowardsEntity