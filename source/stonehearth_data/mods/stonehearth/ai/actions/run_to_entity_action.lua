local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local RunToEntity = class()

RunToEntity.name = 'run to entity'
RunToEntity.does = 'stonehearth:goto_entity'
RunToEntity.args = {
   entity = Entity,
   stop_distance = {
      type = 'number',
      default = 0
   }
}
RunToEntity.think_output = {
   point_of_interest = Point3
}
RunToEntity.version = 2
RunToEntity.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(RunToEntity)
         :execute('stonehearth:find_path_to_entity', {
            destination = ai.ARGS.entity,
         })
         :execute('stonehearth:follow_path', {
            path = ai.PREV.path,
            stop_distance = ai.ARGS.stop_distance
         })
         :set_think_output({
            point_of_interest = ai.BACK(2).path:get_destination_point_of_interest()
         })
