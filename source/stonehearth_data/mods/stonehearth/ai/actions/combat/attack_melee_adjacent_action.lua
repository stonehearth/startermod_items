local AssaultContext = require 'services.server.combat.assault_context'
local BatteryContext = require 'services.server.combat.battery_context'
local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

local AttackMeleeAdjacent = class()

AttackMeleeAdjacent.name = 'attack melee adjacent'
AttackMeleeAdjacent.does = 'stonehearth:combat:attack_melee_adjacent'
AttackMeleeAdjacent.args = {
   target = Entity,
}
AttackMeleeAdjacent.version = 2
AttackMeleeAdjacent.priority = 1
AttackMeleeAdjacent.weight = 1

function AttackMeleeAdjacent:__init(entity)
end

function AttackMeleeAdjacent:start_thinking(ai, entity, args)
   local weapon = stonehearth.combat:get_melee_weapon(entity)

   if weapon == nil or not weapon:is_valid() then
      log:warning('%s has nothing to attack with', entity)
      return
   end

   self._attack_types = stonehearth.combat:get_combat_actions(entity, 'stonehearth:combat:melee_attacks')

   if next(self._attack_types) == nil then
      log:warning('%s has no melee attacks', entity)
      return
   end

   ai:set_think_output()
end

-- TODO: don't allow melee if vertical distance > 1
function AttackMeleeAdjacent:run(ai, entity, args)
   local target = args.target

   if not target:is_valid() then
      log:info('Target has been destroyed')
      ai:abort('Target has been destroyed')
      return
   end

   ai:set_status_text('attacking ' .. radiant.entities.get_name(target))

   local weapon = stonehearth.combat:get_melee_weapon(entity)
   local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
   assert(weapon_data)
   
   local melee_range_ideal, melee_range_max = stonehearth.combat:get_melee_range(entity, weapon_data, target)
   local distance = radiant.entities.distance_between(entity, target)
   if distance > melee_range_max then
      log:warning('%s unable to get within maximum melee range (%f) of %s', entity, melee_range_max, target)
      ai:abort('Target out of melee range')
      return
   end

   local attack_info = stonehearth.combat:choose_combat_action(entity, self._attack_types)

   if not attack_info then
      log:warning('%s unable to attack becuase no melee attacks are currently available.', entity)
      ai:abort('No melee attacks currently available')
      return
   end

   local time_to_impact = stonehearth.combat:get_time_to_impact(attack_info)
   local impact_time = radiant.gamestate.now() + time_to_impact

   radiant.entities.turn_to_face(entity, target)

   ai:execute('stonehearth:bump_against_entity', { entity = target, distance = melee_range_ideal })

   stonehearth.combat:start_cooldown(entity, attack_info)

   self._context = AssaultContext('melee', entity, target, impact_time)
   stonehearth.combat:begin_assault(self._context)

   -- can't ai:execute this. it needs to run in parallel with the attack animation
   self._hit_effect = radiant.effects.run_effect(
      target, '/stonehearth/data/effects/hit_sparks/hit_effect.json', time_to_impact
   )

   self._timer = stonehearth.combat:set_timer(time_to_impact,
      function ()
         local range = radiant.entities.distance_between(entity, target)
         local out_of_range = range > melee_range_max

         if out_of_range or self._context.target_defending then
            self._hit_effect:stop()
         else
            -- TODO: get damage modifiers from action and attributes
            local base_damage = weapon_data.base_damage
            local battery_context = BatteryContext(entity, target, base_damage)
            stonehearth.combat:battery(battery_context)
         end
      end
   )

   ai:execute('stonehearth:run_effect', { effect = attack_info.name })
end

function AttackMeleeAdjacent:stop(ai, entity, args)
   if self._hit_effect ~= nil then
      if radiant.gamestate.now() < self._context.impact_time then
         self._hit_effect:stop()
      end
      self._hit_effect = nil
   end

   if self._timer ~= nil then
      self._timer:destroy()
      self._timer = nil
   end

   if self._context then
      stonehearth.combat:end_assault(self._context)
   end

   self._context = nil
end

return AttackMeleeAdjacent
