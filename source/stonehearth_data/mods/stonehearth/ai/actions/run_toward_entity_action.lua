local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local RunTowardsEntity = class()

RunTowardsEntity.name = 'go towards entity'
RunTowardsEntity.does = 'stonehearth:go_towards_entity'
RunTowardsEntity.args = {
   entity = Entity,
   stop_distance = {
      type = 'number',
      default = 0
   }
}
RunTowardsEntity.think_output = {
   point_of_interest = Point3
}
RunTowardsEntity.version = 2
RunTowardsEntity.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunTowardsEntity)
         :execute('stonehearth:find_direct_path_to_entity', {
            destination = ai.ARGS.entity,
            allow_incomplete_path = true
         })
         :execute('stonehearth:follow_path', {
            path = ai.PREV.path,
            stop_distance = ai.ARGS.stop_distance
         })
         :set_think_output({
            point_of_interest = ai.BACK(2).path:get_destination_point_of_interest()
         })
