local Point3 = _radiant.csg.Point3

local Wander = class()

Wander.name = 'wander'
Wander.does = 'stonehearth:wander'
Wander.args = {
   radius = 'number',     -- how far to wander
   radius_min = {         -- min how far
      type = 'number',
      default = 0,
   }   
}
Wander.version = 2
Wander.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(Wander)
         :execute('stonehearth:choose_point_around_leash', { radius = ai.ARGS.radius, radius_min = ai.ARGS.radius_min })
         :execute('stonehearth:gotoward_location', { location = ai.PREV.location })
