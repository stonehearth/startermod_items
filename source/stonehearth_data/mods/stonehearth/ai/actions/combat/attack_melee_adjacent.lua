local CombatFns = require 'ai.actions.combat.combat_fns'
local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local AttackMeleeAdjacent = class()

AttackMeleeAdjacent.name = 'attack melee adjacent'
AttackMeleeAdjacent.does = 'stonehearth:combat:attack_melee_adjacent'
AttackMeleeAdjacent.args = {
   target = Entity
}
AttackMeleeAdjacent.version = 2
AttackMeleeAdjacent.priority = 1
AttackMeleeAdjacent.weight = 1

function AttackMeleeAdjacent:__init()
   self._weapon_table = CombatFns.get_weapon_table('medium_1h_weapon')
   
   self._attack_types, self._num_attack_types =
      CombatFns.get_action_types(self._weapon_table, 'attack_types')
end

function AttackMeleeAdjacent:start(ai, entity, args)
   self._entity = entity
end

function AttackMeleeAdjacent:run(ai, entity, args)
   local target = args.target
   local roll = rng:get_int(1, self._num_attack_types)
   local attack_name = self._attack_types[roll]
   local log = ai:get_log()
   
   --[[
   if CombatFns.is_baddie(entity) then -- CHECKCHECK
      attack_name = 'combat_power_spike'
   else
      attack_name = 'combat_1h_forehand'
   end
   ]]

   local impact_offset = CombatFns.get_impact_time(self._weapon_table, 'attack_types', attack_name)
   local impact_time = radiant.gamestate.now() + impact_offset

   radiant.entities.turn_to_face(entity, target)

   self:_back_up_to_ideal_attack_range(ai, entity, args)

   self._attack_args = {
      attacker = entity,
      attack_action = self,
      attack_method = 'melee',
      impact_time = impact_time,
   }
   self._defender_combat_action = CombatFns.get_combat_action(target)
   if self._defender_combat_action then
      self._defender_combat_action:begin_assult(self._attack_args)
   end

   -- can't ai:execute this. it needs to run in parallel with the attack animation
   radiant.effects.run_effect(target, 'combat_generic_hit', impact_offset)

   log:debug('starting attack animation "%s"', attack_name)
   ai:execute('stonehearth:run_effect', { effect = attack_name })
   log:debug('finished attack animation "%s"', attack_name)
end

function AttackMeleeAdjacent:_back_up_to_ideal_attack_range(ai, entity, args)
   local target = args.target
   local ideal_separation = CombatFns.get_melee_range(entity, 'medium_1h_weapon', target)
   local current_separation = radiant.entities.distance_between(entity, target)

   if current_separation < ideal_separation then
      ai:execute('stonehearth:bump_against_entity', {
         entity = target,
         distance = ideal_separation
      })
   end
end

function AttackMeleeAdjacent:stop(ai, entity, args)
   self._defender_combat_action = CombatFns.get_combat_action(args.target)
   if self._defender_combat_action then
      self._defender_combat_action:end_assult(self._attack_args)
   end
   self._attack_args = nil
   self._defender_combat_action = nil
end

function AttackMeleeAdjacent:begin_defense(attack_args)
   if attack_args == self._attack_args then
      return {}
   end
end

function AttackMeleeAdjacent:end_defense(attack_args)
   return {}
end

return AttackMeleeAdjacent
