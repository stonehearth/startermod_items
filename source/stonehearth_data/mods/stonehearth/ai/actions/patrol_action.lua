local Entity = _radiant.om.Entity

local Patrol = class()

Patrol.name = 'patrol'
Patrol.status_text_key = 'ai_status_text_patrol'
Patrol.does = 'stonehearth:patrol'
Patrol.args = {}
Patrol.version = 2
Patrol.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Patrol)
         :execute('stonehearth:clear_carrying_now')
         :execute('stonehearth:set_posture', { posture = 'stonehearth:patrol' })
         :execute('stonehearth:add_buff', { buff = 'stonehearth:buffs:patrolling', target = ai.ENTITY})
         :execute('stonehearth:get_patrol_route')
         :execute('stonehearth:follow_path', { path = ai.PREV.path })
         :execute('stonehearth:mark_patrol_completed', { patrollable_object = ai.BACK(2).patrollable_object })
         :execute('stonehearth:patrol:idle')
