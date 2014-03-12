local Point3 = _radiant.csg.Point3

local WanderWithinLeash = class()

WanderWithinLeash.name = 'wander'
WanderWithinLeash.does = 'stonehearth:wander_within_leash'
WanderWithinLeash.args = {
   radius = 'number',     -- how far to wander
   radius_min = {         -- min how far
      type = 'number',
      default = 0,
   }   
}
WanderWithinLeash.version = 2
WanderWithinLeash.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(WanderWithinLeash)
         :execute('stonehearth:choose_point_around_leash', { radius = ai.ARGS.radius, radius_min = ai.ARGS.radius_min })
         :execute('stonehearth:go_toward_location', { location = ai.PREV.location })
