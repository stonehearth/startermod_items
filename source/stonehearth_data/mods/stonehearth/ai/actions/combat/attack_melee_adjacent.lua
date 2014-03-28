local CombatFns = require 'ai.actions.combat.combat_fns'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')
local Entity = _radiant.om.Entity

local AttackMeleeAdjacent = class()

AttackMeleeAdjacent.name = 'attack melee adjacent'
AttackMeleeAdjacent.does = 'stonehearth:attack_melee_adjacent'
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
   radiant.events.listen(entity, 'stonehearth:hit_defended', self, self._on_battery_defended)
end

function AttackMeleeAdjacent:run(ai, entity, args)
   local target = args.target
   local roll = rng:get_int(1, self._num_attack_types)
   local attack_name = self._attack_types[roll]
   local impact_time = CombatFns.get_impact_time(self._weapon_table, 'attack_types', attack_name)

   radiant.entities.turn_to_face(entity, target)

   self:_move_to_ideal_attack_range(ai, entity, args)

   radiant.events.trigger(target, 'stonehearth:hit', {
      attacker = entity,
      impact_time = impact_time
   })

   -- trigger and response (or lack of response) execute synchronously
   if not self._defended then
      -- can't ai:execute this. it needs to run in parallel with the attack animation
      radiant.effects.run_effect(target, 'combat_generic_hit', impact_time)
   end

   ai:execute('stonehearth:run_effect', { effect = attack_name })
end

function AttackMeleeAdjacent:_move_to_ideal_attack_range(ai, entity, args)
   local target = args.target
   local ideal_separation = CombatFns.get_melee_range(entity, 'medium_1h_weapon', target)
   local current_separation = self:_distance_between(entity, target)

   if current_separation < ideal_separation then
      ai:execute('stonehearth:bump_against_entity', {
         entity = target,
         distance = ideal_separation
      })
   end
end

function AttackMeleeAdjacent:stop(ai, entity, args)
   radiant.events.unlisten(entity, 'stonehearth:hit_defended', self, self._on_battery_defended)
end

function AttackMeleeAdjacent:_on_battery_defended(args)
   self._defended = true
end

-- need non-grid aligned distance
function AttackMeleeAdjacent:_distance_between(entity1, entity2)
   local location1 = entity1:add_component('mob'):get_world_location()
   local location2 = entity2:add_component('mob'):get_world_location()
   return location1:distance_to(location2)
end

return AttackMeleeAdjacent
