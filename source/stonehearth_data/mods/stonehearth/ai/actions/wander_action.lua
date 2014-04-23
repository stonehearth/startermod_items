local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local Wander = class()

Wander.name = 'wander'
Wander.does = 'stonehearth:wander'
Wander.args = {
   radius = 'number',     -- how far to wander
   radius_min = {         -- min how far
      type = 'number',
      default = 0
   }   
}
Wander.version = 2
Wander.priority = 1

function Wander:run(ai, entity, args)
   local entity_location = radiant.entities.get_world_grid_location(entity)
   local radius = args.radius
   local radius_min = args.radius_min

   local dx = rng:get_int(radius_min, radius)
   if rng:get_int(1, 2) == 1 then
      dx = dx * -1
   end

   local dz = rng:get_int(radius_min, radius)
   if rng:get_int(1, 2) == 1 then
      dz = dz * -1
   end

   local destination = Point3(
      entity_location.x + dx,
      entity_location.y,
      entity_location.z + dz
   )

   ai:execute('stonehearth:go_toward_location', { destination = destination })
end

return Wander
