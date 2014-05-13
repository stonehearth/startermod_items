local Point3 = _radiant.csg.Point3
local RunTowardLocation = class()

RunTowardLocation.name = 'run toward location'
RunTowardLocation.does = 'stonehearth:go_toward_location'
RunTowardLocation.args = {
   destination = Point3,
   move_effect = {
      type = 'string',
      default = 'run',
   },
}
RunTowardLocation.version = 2
RunTowardLocation.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RunTowardLocation)
         :execute('stonehearth:find_direct_path_to_location', {
            destination = ai.ARGS.destination,
            allow_incomplete_path = true,
            reversible_path = true,
         })
         :execute('stonehearth:follow_path', {
            path = ai.PREV.path,
            move_effect = ai.ARGS.move_effect,
         })
