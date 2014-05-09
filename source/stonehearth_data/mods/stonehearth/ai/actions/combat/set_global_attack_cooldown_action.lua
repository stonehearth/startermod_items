local log = radiant.log.create_logger('combat')

local SetGlobalAttackCooldown = class()

SetGlobalAttackCooldown.name = 'set global attack cooldown'
SetGlobalAttackCooldown.does = 'stonehearth:combat:set_global_attack_cooldown'
SetGlobalAttackCooldown.args = {}
SetGlobalAttackCooldown.version = 2
SetGlobalAttackCooldown.priority = 1

function SetGlobalAttackCooldown:__init()
   self._global_attack_recovery_cooldown = radiant.util.get_config('global_attack_recovery_cooldown', 1000)
end

function SetGlobalAttackCooldown:run(ai, entity, args)
end

function SetGlobalAttackCooldown:stop(ai, entity, args)
   local state = stonehearth.combat:get_combat_state(entity)

   state:start_cooldown('global_attack_recovery', self._global_attack_recovery_cooldown)
end

return SetGlobalAttackCooldown
