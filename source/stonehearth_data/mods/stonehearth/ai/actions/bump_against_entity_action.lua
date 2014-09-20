local AiHelpers = require 'ai.actions.ai_helpers'
local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local BumpAgainstEntity = class()
BumpAgainstEntity.name = 'bump against entity'
BumpAgainstEntity.does = 'stonehearth:bump_against_entity'
BumpAgainstEntity.args = {
   entity = Entity,      -- entity to bump against
   distance = 'number'   -- separation distance (as measured from both entity centers)
}
BumpAgainstEntity.version = 2
BumpAgainstEntity.priority = 1

function BumpAgainstEntity:run(ai, entity, args)
   local distance = args.distance
   local bumper = args.entity
   local bumpee = entity

   local vector = AiHelpers.calculate_bump_vector(bumper, bumpee, distance)

   if vector ~= Point3.zero then
      ai:execute('stonehearth:bump_entity', {
         vector = vector
      })
   end
end

return BumpAgainstEntity
