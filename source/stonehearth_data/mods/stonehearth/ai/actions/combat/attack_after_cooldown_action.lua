local constants = require 'constants'
local Entity = _radiant.om.Entity

local EngageAndAttack = class()
EngageAndAttack.name = 'attack after cooldown'
EngageAndAttack.does = 'stonehearth:combat:attack_after_cooldown'
EngageAndAttack.args = {
   target = Entity
}
EngageAndAttack.version = 2
EngageAndAttack.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(EngageAndAttack)
   -- note that global attack (recovery) cooldowns start after an attack finishes
   :execute('stonehearth:combat:wait_for_global_attack_cooldown')
   :execute('stonehearth:combat:attack', { target = ai.ARGS.target })
   :execute('stonehearth:combat:set_global_attack_cooldown')
