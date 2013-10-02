local RunAwayFromEntity = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

RunAwayFromEntity.name = 'run away from an entity'
RunAwayFromEntity.does = 'stonehearth.run_away_from_entity'
RunAwayFromEntity.priority = 1

function RunAwayFromEntity:run(ai, entity, target)
   assert(target)

   local my_location = radiant.entities.get_world_grid_location(entity)
   local target_location = radiant.entities.get_world_grid_location(target)

   local flee_vector = Vec3(target_location.x - my_location.x, my_location.y, target_location.z - my_location.z)
   flee_vector:normalize()

   flee_vector.x = self:round(flee_vector.x) * -2
   flee_vector.z = self:round(flee_vector.z) * -2

   local flee_location = Point3(my_location.x + flee_vector.x, my_location.y, my_location.z + flee_vector.z)

   ai:execute('stonehearth.goto_location', flee_location)
end

function RunAwayFromEntity:round(num, idp)
  local mult = 10^(idp or 0)
  return math.floor(num * mult + 0.5) / mult
end

return RunAwayFromEntity