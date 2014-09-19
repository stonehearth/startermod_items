local AiHelpers = require 'ai.actions.ai_helpers'
local Point3 = _radiant.csg.Point3

local BumpEntity = class()
BumpEntity.name = 'bump entity'
BumpEntity.does = 'stonehearth:bump_entity'
BumpEntity.args = {
   vector = Point3   -- distance and direction to bump
}
BumpEntity.version = 2
BumpEntity.priority = 1

function BumpEntity:run(ai, entity, args)
   AiHelpers.bump_entity(entity, args.vector)
end

return BumpEntity
