local Entity = _radiant.om.Entity

local Patrol = class()

Patrol.name = 'patrol'
Patrol.does = 'stonehearth:patrol'
Patrol.args = {
   waypoint = Entity,
}
Patrol.version = 2
Patrol.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Patrol)
         :execute('stonehearth:set_posture', { posture = 'combat' })
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.waypoint,
            stop_distance = 4,
         })
         :execute('stonehearth:walk_around_entity', {
            target = ai.ARGS.waypoint,
            distance = 2,
         })
         :execute('stonehearth:unset_posture', { posture = 'combat' })
