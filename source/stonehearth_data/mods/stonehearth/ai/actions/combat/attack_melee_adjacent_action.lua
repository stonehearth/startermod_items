local AssaultContext = require 'services.server.combat.assault_context'
local HitStunContext = require 'services.server.combat.hit_stun_context'
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
   self._weapon_table = stonehearth.combat:get_weapon_table('medium_1h_weapon')
   
   self._attack_types = stonehearth.combat:get_action_types(self._weapon_table, 'attack_types')
   self._num_attack_types = #self._attack_types
end

function AttackMeleeAdjacent:run(ai, entity, args)
   local target = args.target

   if not target:is_valid() then
      ai:abort('enemy has been destroyed')
      return
   end

   local attack_info = stonehearth.combat:choose_action(entity, self._attack_types)
   local time_to_impact = stonehearth.combat:get_time_to_impact(attack_info)
   local impact_time = radiant.gamestate.now() + time_to_impact

   radiant.entities.turn_to_face(entity, target)

   local melee_range = stonehearth.combat:get_melee_range(entity, 'medium_1h_weapon', target)
   ai:execute('stonehearth:bump_against_entity', { entity = target, distance = melee_range })

   stonehearth.combat:start_cooldown(entity, attack_info)

   self._context = AssaultContext('melee', entity, impact_time)
   stonehearth.combat:assault(target, self._context)

   -- can't ai:execute this. it needs to run in parallel with the attack animation
   self._hit_effect = radiant.effects.run_effect(target, 'combat_generic_hit', time_to_impact)

   self._timer = stonehearth.combat:set_timer(time_to_impact,
      function ()
         if self._context.target_defending then
            self._hit_effect:stop()
         else
            -- TODO: negotiate damage with defender and get appropriate effect
            stonehearth.combat:hit_stun(target, HitStunContext())
         end
      end
   )

   ai:execute('stonehearth:run_effect', { effect = attack_info.name })
end

function AttackMeleeAdjacent:stop(ai, entity, args)
   if radiant.gamestate.now() < self._context.impact_time then
      self._hit_effect:stop()
   end

   if self._timer ~= nil then
      self._timer:destroy()
      self._timer = nil
   end

   self._context = nil
end

return AttackMeleeAdjacent
