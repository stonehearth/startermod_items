local RunToEntity = class()

RunToEntity.name = 'walk to entity'
RunToEntity.does = 'stonehearth:goto_entity'
RunToEntity.args = {
   _radiant.om.Entity,     -- entity to travel to
   'string',               -- default effect to use when travelling
}
RunToEntity.version = 2
RunToEntity.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(RunToEntity)
         :execute('stonehearth:find_path_to_entity', ai.ARGS[1])
         :execute('stonehearth:follow_path', ai.PREV[1], ai.ARGS[2])
