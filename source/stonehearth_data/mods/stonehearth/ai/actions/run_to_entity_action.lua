local Entity = _radiant.om.Entity
local RunToEntity = class()

RunToEntity.name = 'run to entity'
RunToEntity.does = 'stonehearth:goto_entity'
RunToEntity.args = {
   entity = Entity,     		-- entity to travel to
   effect = 'string',      -- default effect to use when travelling
}
RunToEntity.version = 2
RunToEntity.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(RunToEntity)
         :execute('stonehearth:find_path_to_entity', { entity = ai.ARGS.entity })
         :execute('stonehearth:follow_path', { path = ai.PREV.path, effect = ai.ARGS.effect })
