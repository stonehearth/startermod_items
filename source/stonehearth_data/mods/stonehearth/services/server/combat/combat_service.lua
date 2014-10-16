local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

CombatService = class()

function CombatService:__init()
end

function CombatService:initialize()
   -- always stun for now
   --self._hit_stun_damage_threshold = radiant.util.get_config('hit_stun_damage_threshold', 0.10)
   self._hit_stun_damage_threshold = 0

   self:_register_score_functions()
end

function CombatService:_register_score_functions()
   --If the entity is a farm, register the score
   stonehearth.score:add_aggregate_eval_function('military_strength', 'military_strength', function(entity, agg_score_bag)
      local weapon = stonehearth.combat:get_melee_weapon(entity)
      if not weapon then
         return
      end

      local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
      if not weapon_data then
         return
      end

      agg_score_bag.military_strength = agg_score_bag.military_strength + weapon_data.base_damage
   end)
end

-- Notify target that it is about to be attacked.
-- If target is not otherwise engaged in another combat action, target should stop
-- and defend itself, which allows attacker to close to the proper distance.
function CombatService:engage(context)
   local target = context.target

   if not target or not target:is_valid() then
      return nil
   end

   radiant.events.trigger_async(target, 'stonehearth:combat:engage', context)
end

-- Notify target that an attack has begun and will impact soon.
-- Target has opportunity to defend itself if it can react before the impact time.
function CombatService:begin_assault(context)
   local attacker = context.attacker
   local target = context.target

   if not target or not target:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(target)
   combat_state:add_assault_event(context)

   radiant.events.trigger_async(attacker, 'stonehearth:combat:begin_melee_attack', context)
   radiant.events.trigger_async(target, 'stonehearth:combat:assault', context)
   self:_set_assaulting(attacker, true)
end

function CombatService:end_assault(context)
   local attacker = context.attacker
   local target = context.target

   if target and target:is_valid() then
      local combat_state = self:get_combat_state(target)
      combat_state:remove_assault_event(context)
   end

   self:_set_assaulting(attacker, false)
   context.assault_active = false
end

-- Notify target that it has been hit by an attack.
function CombatService:battery(context)
   local attacker = context.attacker
   local target = context.target

   if not target or not target:is_valid() then
      return nil
   end

   local attributes_component = target:add_component('stonehearth:attributes')
   local max_health = attributes_component:get_attribute('max_health')
   local health = attributes_component:get_attribute('health')
   local damage = context.damage
   local target_exp = attributes_component:get_attribute('exp_per_hit')

   if max_health ~= nil then
      if damage >= max_health * self._hit_stun_damage_threshold then
         self:hit_stun(context)
      end
   end

   if health ~= nil then
      health = health - damage
      attributes_component:set_attribute('health', health)

      --If health falls below 0, someone has died. That seems significant!
   end

   local action_details = {
      attacker = attacker,
      target = target,
      damage = damage, 
      target_health = health, 
      target_exp = target_exp
   }
   radiant.events.trigger_async(radiant, 'stonehearth:combat:combat_action', action_details)
   radiant.events.trigger_async(attacker, 'stonehearth:combat:melee_attack_connect', action_details)
   radiant.events.trigger_async(target, 'stonehearth:combat:battery', context)
end

-- Notify target that it is now stunned an any action in progress will be cancelled.
function CombatService:hit_stun(context)
   local target = context.target

   if not target or not target:is_valid() then
      return nil
   end

   radiant.events.trigger_async(target, 'stonehearth:combat:hit_stun', context)
end

function CombatService:get_assault_events(target)
   if not target or not target:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(target)
   return combat_state:get_assault_events()
end

function CombatService:start_cooldown(entity, action_info)
   if not entity or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   combat_state:start_cooldown(action_info.name, action_info.cooldown)
end

function CombatService:in_cooldown(entity, action_name)
   if not entity or not entity:is_valid() then
      return false
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:in_cooldown(action_name)
end

function CombatService:get_cooldown_end_time(entity, action_name)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_cooldown_end_time(action_name)
end

function CombatService:get_melee_weapon(entity)
   return radiant.entities.get_equipped_item(entity, 'mainhand')
end

function CombatService:get_melee_range(attacker, weapon_data, target)
   local attacker_reach = self:get_entity_reach(attacker)
   local target_radius = self:get_entity_radius(target)
   local weapon_reach = weapon_data.reach
   local melee_range_ideal = attacker_reach + weapon_reach + target_radius
   local melee_range_max = melee_range_ideal + 2
   return melee_range_ideal, melee_range_max
end

function CombatService:get_engage_range(attacker, weapon_data, target)
   local melee_range_ideal = self:get_melee_range(attacker, weapon_data, target)
   local engage_range_ideal = melee_range_ideal + 3
   local engage_range_max = engage_range_ideal + 3
   return engage_range_ideal, engage_range_max
end

function CombatService:get_entity_reach(entity)
   local entity_reach = 0

   if entity and entity:is_valid() then
      entity_reach = radiant.entities.get_entity_data(entity, 'stonehearth:entity_reach')
      if not entity_reach then
         log:error('%s does not have entity data stonehearth:entity_reach. Using default', entity)
         entity_reach = 1.0
      end
   end

   return entity_reach
end

function CombatService:get_entity_radius(entity)
   local entity_radius = 0

   if entity and entity:is_valid() then
      entity_radius = radiant.entities.get_entity_data(entity, 'stonehearth:entity_radius')
      if not entity_radius then
         log:error('%s does not have entity data stonehearth:entity_radius. Using default.', entity)
         entity_radius = 0.5
      end
   end

   return entity_radius
end

-- used for synchronizing animation
function CombatService:ms_to_game_seconds(ms)
   return ms / stonehearth.calendar:get_constants().ticks_per_second
end

-- duration is in milliseconds at game speed 1
function CombatService:set_timer(duration, fn)
   local game_seconds = self:ms_to_game_seconds(duration)
   return stonehearth.calendar:set_timer(game_seconds, fn)
end

function CombatService:get_primary_target(entity)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_primary_target()
end

function CombatService:set_primary_target(entity, target)
   if not entity or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:set_primary_target(target)
end

function CombatService:get_assaulting(entity)
   if not entity or not entity:is_valid() then
      return false
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_assaulting()
end

-- external parties should be using the begin_assault and end_assault methods
function CombatService:_set_assaulting(entity, assaulting)
   if not entity or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:set_assaulting(assaulting)
end

function CombatService:get_stance(entity)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_stance()
end

function CombatService:set_stance(entity, stance)
   if not entity or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:set_stance(stance)
end

function CombatService:get_panicking_from(entity)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_panicking_from()
end

function CombatService:set_panicking_from(entity, threat)
   if not entity or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   combat_state:set_panicking_from(threat)
end

function CombatService:panicking(entity)
   if not entity or not entity:is_valid() then
      return false
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:panicking()
end

function CombatService:get_combat_state(entity)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state_component = entity:add_component('stonehearth:combat_state')
   return combat_state_component
end

function CombatService:get_combat_actions(entity, action_type)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_combat_actions(action_type)
end

-- get the highest priority action that is ready now
-- assumes actions are sorted by descending priority
function CombatService:choose_attack_action(entity, actions)
   local filter_fn = function(combat_state, action_info)
      return not combat_state:in_cooldown(action_info.name)
   end
   return self:_choose_combat_action(entity, actions, filter_fn)
end

-- get the highest priority action that can take effect before the impact_time
-- assumes actions are sorted by descending priority
function CombatService:choose_defense_action(entity, actions, attack_impact_time)
   local filter_fn = function(combat_state, action_info)
      local ready_time = combat_state:get_cooldown_end_time(action_info.name) or radiant.gamestate.now()
      local defense_impact_time = ready_time + action_info.time_to_impact
      return defense_impact_time <= attack_impact_time
   end
   return self:_choose_combat_action(entity, actions, filter_fn)
end

-- assumes actions are sorted by descending priority
function CombatService:_choose_combat_action(entity, actions, filter_fn)
   local combat_state = self:get_combat_state(entity)
   local candidates = {}
   local priority = nil

   for _, action_info in ipairs(actions) do
      if filter_fn(combat_state, action_info) then
         if not priority or action_info.priority == priority then
            -- add the available action and any other actions at the same priority
            table.insert(candidates, action_info)
            priority = action_info.priority
         else
            -- lower priority action, nothing else to find (recall that actions is sorted by priority)
            assert(action_info.priority < priority)
            break
         end
      end
   end

   local num_candidates = #candidates

   if num_candidates == 0 then
      return nil
   end

   local roll = rng:get_int(1, num_candidates)
   return candidates[roll]
end

return CombatService
