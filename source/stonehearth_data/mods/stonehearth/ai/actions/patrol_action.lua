local Entity = _radiant.om.Entity

local Patrol = class()

Patrol.name = 'patrol'
Patrol.status_text = 'patrolling'
Patrol.does = 'stonehearth:work'
Patrol.args = {}
Patrol.version = 2
Patrol.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Patrol)
         :execute('stonehearth:set_posture', { posture = 'stonehearth:combat' })
         :execute('stonehearth:get_patrol_route')
         :execute('stonehearth:follow_path', { path = ai.PREV.path })
         :execute('stonehearth:mark_patrol_completed', { patrollable_object = ai.BACK(2).patrollable_object })
         :execute('stonehearth:patrol:idle')
         :execute('stonehearth:unset_posture', { posture = 'stonehearth:combat' })
