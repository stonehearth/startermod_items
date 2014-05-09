local constants = require 'constants'
local Entity = _radiant.om.Entity

local CombatAttackAction = class()
CombatAttackAction.name = 'combat attack action'
CombatAttackAction.does = 'stonehearth:combat'
CombatAttackAction.args = {
   enemy = Entity
}
CombatAttackAction.version = 2
CombatAttackAction.priority = constants.priorities.combat.ACTIVE

local ai = stonehearth.ai
return ai:create_compound_action(CombatAttackAction)
   -- note that global attack (recovery) cooldowns start after an attack finishes
   :execute('stonehearth:combat:wait_for_global_attack_cooldown')
   :execute('stonehearth:combat:attack', { target = ai.ARGS.enemy })
   :execute('stonehearth:combat:set_global_attack_cooldown')
