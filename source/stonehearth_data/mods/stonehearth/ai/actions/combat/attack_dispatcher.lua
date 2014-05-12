local constants = require 'constants'
local Entity = _radiant.om.Entity

local AttackDispatcher = class()
AttackDispatcher.name = 'attack dispatcher'
AttackDispatcher.does = 'stonehearth:combat'
AttackDispatcher.args = {
   enemy = Entity
}
AttackDispatcher.version = 2
AttackDispatcher.priority = constants.priorities.combat.ACTIVE

local ai = stonehearth.ai
return ai:create_compound_action(AttackDispatcher)
   -- note that global attack (recovery) cooldowns start after an attack finishes
   :execute('stonehearth:combat:wait_for_global_attack_cooldown')
   :execute('stonehearth:combat:attack', { target = ai.ARGS.enemy })
   :execute('stonehearth:combat:set_global_attack_cooldown')
