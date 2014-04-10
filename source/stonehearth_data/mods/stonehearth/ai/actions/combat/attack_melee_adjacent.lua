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
   self._defended = false
   radiant.events.listen(entity, 'stonehearth:combat:assault_defended', self, self._on_assault_defended)
end

function AttackMeleeAdjacent:run(ai, entity, args)
   local target = args.target
   local roll = rng:get_int(1, self._num_attack_types)
   local attack_name = self._attack_types[roll]
   local log = ai:get_log()
   
   if CombatFns.is_baddie(entity) then -- CHECKCHECK
      attack_name = 'combat_power_spike'
   else
      attack_name = 'combat_1h_forehand'
   end

   local impact_time = CombatFns.get_impact_time(self._weapon_table, 'attack_types', attack_name)

   radiant.entities.turn_to_face(entity, target)

   self:_back_up_to_ideal_attack_range(ai, entity, args)

   radiant.events.trigger(target, 'stonehearth:combat:assault', {
      attack_method = 'melee',
      attacker = entity,
      impact_time = impact_time
   })

   -- trigger and response (or lack of response) execute synchronously
   if not self._defended then
      -- can't ai:execute this. it needs to run in parallel with the attack animation
      radiant.effects.run_effect(target, 'combat_generic_hit', impact_time)
   end

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
   radiant.events.unlisten(entity, 'stonehearth:combat:assault_defended', self, self._on_assault_defended)
end

function AttackMeleeAdjacent:_on_assault_defended(args)
   self._defended = true
end

return AttackMeleeAdjacent
