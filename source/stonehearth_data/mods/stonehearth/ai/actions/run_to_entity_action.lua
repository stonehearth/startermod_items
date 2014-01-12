local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local RunToEntity = class()

RunToEntity.name = 'run to entity'
RunToEntity.does = 'stonehearth:goto_entity'
RunToEntity.args = {
   entity = Entity,           -- entity to travel to
}
RunToEntity.think_output = {
   point_of_interest = Point3
}
RunToEntity.version = 2
RunToEntity.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(RunToEntity)
         :execute('stonehearth:find_path_to_entity', { finish = ai.ARGS.entity })
         :execute('stonehearth:follow_path', { path = ai.PREV.path })
         :set_think_output({ point_of_interest = ai.BACK(2).path:get_destination_point_of_interest() })
