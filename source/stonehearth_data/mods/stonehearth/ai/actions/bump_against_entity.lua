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

local function random_xz_unit_vector()
   local unit_x = Point3f(1, 0, 0)
   local unit_y = Point3f(0, 1, 0)
   local angle = rng:get_real(0, 2 * math.pi)
   local vector = Quaternion(unit_y, angle):rotate(unit_x)
   return vector
end

function BumpAgainstEntity:run(ai, entity, args)
   local distance = args.distance
   local bumper = args.entity

   if bumper == nil or not bumper:is_valid() then
      return
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
      vector = random_xz_unit_vector()
   end

   vector:scale(bump_distance)

   ai:execute('stonehearth:bump_entity', {
      vector = vector
   })
end

return BumpAgainstEntity
