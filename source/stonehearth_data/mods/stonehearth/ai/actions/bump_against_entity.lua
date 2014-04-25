local Point3f = _radiant.csg.Point3f
local Quaternion = _radiant.csg.Quaternion
local rng = _radiant.csg.get_default_rng()
local Entity = _radiant.om.Entity

local BumpAgainstEntity = class()
BumpAgainstEntity.name = 'bump against entity'
BumpAgainstEntity.does = 'stonehearth:bump_against_entity'
BumpAgainstEntity.args = {
   entity = Entity,      -- entity to bump against
   distance = 'number'   -- separation distance (as measeured from both entity centers)
}
BumpAgainstEntity.version = 2
BumpAgainstEntity.priority = 1

function BumpAgainstEntity:run(ai, entity, args)
   local distance = args.distance
   local bumper = args.entity
   local mob = entity:get_component('mob')
   
   if not mob then
      ai:abort('%s is not a mob', tostring(entity))
   end

   local entity_location = entity:add_component('mob'):get_world_location()
   local bumper_location = bumper:add_component('mob'):get_world_location()
   local current_separation = entity_location:distance_to(bumper_location)
   local bump_distance = distance - current_separation

   if bump_distance <= 0 then
      return
   end

   local vector = entity_location - bumper_location
   vector.y = 0   -- only x,z bumping permitted

   if vector:distance_squared() ~= 0 then
      vector:normalize()
   else
      -- pick a random direction (rotate the unit_x vector around the y-axis by a random angle)
      local unit_x = Point3f(1, 0, 0)
      local unit_y = Point3f(0, 1, 0)
      local angle = rng:get_real(0, 2 * math.pi)
      vector = Quaternion(unit_y, angle):rotate(unit_x)
   end

   vector:scale(bump_distance)

   ai:execute('stonehearth:bump_entity', {
      vector = vector
   })
end

return BumpAgainstEntity
