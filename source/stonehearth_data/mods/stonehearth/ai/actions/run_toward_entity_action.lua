local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local RunTowardEntity = class()

RunTowardEntity.name = 'run toward entity'
RunTowardEntity.does = 'stonehearth:go_toward_entity'
RunTowardEntity.args = {
   entity = Entity,
   stop_distance = {
      type = 'number',
      default = 0
   }
}
RunTowardEntity.think_output = {
   point_of_interest = Point3
}
RunTowardEntity.version = 2
RunTowardEntity.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunTowardEntity)
         :execute('stonehearth:find_direct_path_to_entity', {
            destination = ai.ARGS.entity,
            allow_incomplete_path = true,
            reversible_path = true
         })
         :execute('stonehearth:follow_path', {
            path = ai.PREV.path,
            stop_distance = ai.ARGS.stop_distance
         })
         :set_think_output({
            point_of_interest = ai.BACK(2).path:get_destination_point_of_interest()
         })
