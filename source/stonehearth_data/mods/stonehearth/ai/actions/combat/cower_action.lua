local Entity = _radiant.om.Entity

local Cower = class()
Cower.name = 'combat cower'
Cower.status_text = 'cowering'
Cower.does = 'stonehearth:combat:panic'
Cower.args = {
   threat = Entity,
}
Cower.version = 2
Cower.priority = 1
Cower.weight = 2

local ai = stonehearth.ai
return ai:create_compound_action(Cower)
   :execute('stonehearth:set_posture', { posture = 'stonehearth:cower' })
   :execute('stonehearth:run_effect', { effect = 'cower', times = 20 })
   :execute('stonehearth:unset_posture', { posture = 'stonehearth:cower' })
