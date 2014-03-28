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
   vector:normalize()
   vector:scale(bump_distance)

   ai:execute('stonehearth:bump_entity', {
      vector = vector
   })
end

return BumpAgainstEntity
