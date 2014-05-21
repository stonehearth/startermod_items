local Entity = _radiant.om.Entity

local Patrol = class()

Patrol.name = 'patrol'
Patrol.does = 'stonehearth:work'
Patrol.args = {}
Patrol.version = 2
Patrol.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Patrol)
         :execute('stonehearth:set_posture', { posture = 'combat' })
         :execute('stonehearth:get_patrol_point')
         :execute('stonehearth:goto_location', { location = ai.PREV.destination })
         :execute('stonehearth:mark_patrol_complete')
         :execute('stonehearth:patrol:idle')
         :execute('stonehearth:unset_posture', { posture = 'combat' })
