local CombatFns = require 'ai.actions.combat.combat_fns'
local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local AttackMelee = class()

AttackMelee.name = 'melee attack'
AttackMelee.does = 'stonehearth:combat:attack'
AttackMelee.args = {
   target = Entity
}
AttackMelee.version = 2
AttackMelee.priority = 1
AttackMelee.weight = 1

function AttackMelee:run(ai, entity, args)
   local target = args.target
   local melee_range = CombatFns.get_melee_range(entity, 'medium_1h_weapon', target)
   local engage_range = melee_range + 2
   local engaged = false

   while true do
      local distance = radiant.entities.distance_between(entity, target)

      if distance <= engage_range then
         radiant.events.trigger(target, 'stonehearth:combat:engage', {
            attacker = entity
         })
         engaged = true
      end

      if distance > melee_range then
         --ai:execute('stonehearth:goto_entity', { entity = target, stop_distance = melee_range })
         ai:execute('stonehearth:chase_entity', { target = target, stop_distance = melee_range })
      end

      if engaged then
         break
      end
   end

   ai:execute('stonehearth:combat:attack_melee_adjacent', { target = target })
end

return AttackMelee
