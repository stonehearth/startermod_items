local Point3 = _radiant.csg.Point3
local RunToClosestStandableLocation = class()

RunToClosestStandableLocation.name = 'run to closest standable location'
RunToClosestStandableLocation.does = 'stonehearth:goto_closest_standable_location'
RunToClosestStandableLocation.args = {
   location = Point3,
   max_radius = 'number',
}
RunToClosestStandableLocation.version = 2
RunToClosestStandableLocation.priority = 1

function RunToClosestStandableLocation:start_thinking(ai, entity, args)
   local resolved_location, found = radiant.terrain.find_closest_standable_point_to(
                                    args.location, args.max_radius, entity)

   if found then
      ai:set_think_output({ location = resolved_location })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(RunToClosestStandableLocation)
         :execute('stonehearth:goto_location', {
            location = ai.PREV.location,
         })
