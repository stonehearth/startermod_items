local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local CombatIdle = class()

CombatIdle.name = 'combat idle'
CombatIdle.does = 'stonehearth:combat:idle'
CombatIdle.args = {
   facing_entity = Entity
}
CombatIdle.version = 2
CombatIdle.priority = 1
CombatIdle.weight = 1

function CombatIdle:run(ai, entity, args)
   -- TODO: get the idle animation appropriate to the weapon
   ai:execute('stonehearth:run_effect', {
      effect = 'combat_1h_idle'
   })
end

return CombatIdle
