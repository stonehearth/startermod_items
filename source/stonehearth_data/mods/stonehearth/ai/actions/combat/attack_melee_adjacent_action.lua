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

   -- refetch every start_thinking as the set of actions may have changed
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

   local attack_info = stonehearth.combat:choose_attack_action(entity, self._attack_types)

   if not attack_info then
      log:warning('%s unable to attack becuase no melee attacks are currently available.', entity)
      ai:abort('No melee attacks currently available')
      return
   end

   radiant.entities.turn_to_face(entity, target)

   ai:execute('stonehearth:bump_against_entity', { entity = target, distance = melee_range_ideal })

   stonehearth.combat:start_cooldown(entity, attack_info)

   -- the target might die when we attack them, so unprotect now!
   ai:unprotect_entity(target)
   
   local impact_time = radiant.gamestate.now() + attack_info.time_to_impact
   self._assault_context = AssaultContext('melee', entity, target, impact_time)
   stonehearth.combat:begin_assault(self._assault_context)

   -- can't ai:execute this. it needs to run in parallel with the attack animation
   self._hit_effect = radiant.effects.run_effect(
      target, '/stonehearth/data/effects/hit_sparks/hit_effect.json', attack_info.time_to_impact
   )

   self._timer = stonehearth.combat:set_timer(attack_info.time_to_impact,
      function ()
         if not entity:is_valid() or not target:is_valid() then
            return
         end
         
         local range = radiant.entities.distance_between(entity, target)
         local out_of_range = range > melee_range_max

         if out_of_range or self._assault_context.target_defending then
            self._hit_effect:stop()
            self._hit_effect = nil
         else
            -- TODO: get damage modifiers from action and attributes
            local base_damage = weapon_data.base_damage

            -- TODO: Implement system to collect all damage types and all armor types
            -- and then resolve to compute the final damage type. 
            -- TODO: figure out HP progression of enemies, so this system will scale well
            -- For example, if you melee Cthulu what elements should be in play so a high lv footman
            -- will be able to actually make a difference?
            -- For now, will have an additive dmg attribute, a multiplicative dmg attribute
            -- and will apply both to this base damage number
            -- TODO: Albert to implement more robust solution after he works on mining
            local total_damage = self:_calculate_total_damage(entity, base_damage)
            local battery_context = BatteryContext(entity, target, total_damage)
            stonehearth.combat:battery(battery_context)
         end
      end
   )

   ai:execute('stonehearth:run_effect', { effect = attack_info.name })
end

-- TODO: modify to work with final design
-- TODO: should base damage be a range? Right now it is fixed by weapon.
function AttackMeleeAdjacent:_calculate_total_damage(entity, base_damage)
   local total_damage = base_damage
   local attributes_component = entity:get_component('stonehearth:attributes')
   if not attributes_component then 
      return total_damage
   end
   local additive_dmg_modifier = attributes_component:get_attribute('additive_dmg_modifier')
   local multiplicative_dmg_modifier = attributes_component:get_attribute('multiplicative_dmg_modifier')
   if multiplicative_dmg_modifier then
      local dmg_to_add = base_damage * multiplicative_dmg_modifier
      total_damage = dmg_to_add + total_damage
   end
   if additive_dmg_modifier then
      total_damage = total_damage + additive_dmg_modifier
   end
   return total_damage
end

function AttackMeleeAdjacent:stop(ai, entity, args)
   if self._hit_effect ~= nil then
      if radiant.gamestate.now() < self._assault_context.impact_time then
         self._hit_effect:stop()
      end
      self._hit_effect = nil
   end

   if self._timer ~= nil then
      -- cancel the timer if we were pre-empted
      self._timer:destroy()
      self._timer = nil
   end

   if self._assault_context then
      stonehearth.combat:end_assault(self._assault_context)
   end

   self._assault_context = nil
end

return AttackMeleeAdjacent
