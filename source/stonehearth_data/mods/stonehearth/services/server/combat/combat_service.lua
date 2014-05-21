local CombatState = require 'components.combat_state.combat_state'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

CombatService = class()

local MS_PER_FRAME = 1000/30

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
function CombatService:engage(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   radiant.events.trigger_async(target, 'stonehearth:combat:engage', context)
end

-- Notify target that an attack has begun and will impact soon.
-- Target has opportunity to defend itself if it can react before the impact time.
function CombatService:assault(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(target)
   combat_state:add_assault_event(context)

   radiant.events.trigger_async(target, 'stonehearth:combat:assault', context)
end

-- Notify target that it has been hit by an attack.
function CombatService:battery(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   local attributes_component = target:add_component('stonehearth:attributes')
   local max_health = attributes_component:get_attribute('max_health')
   local health = attributes_component:get_attribute('health')
   local damage = context.damage

   if max_health ~= nil then
      if damage >= max_health * self._hit_stun_damage_threshold then
         self:hit_stun(target, context)
      end
   end

   health = health - damage
   if health ~= nil then
      attributes_component:set_attribute('health', health)
   end

   local action_details = {
      attacker = context.attacker,
      target = target,
      damage = damage
   }
   radiant.events.trigger_async(radiant, 'stonehearth:combat:combat_action', action_details)
   radiant.events.trigger_async(target, 'stonehearth:combat:battery', context)
end

-- Notify target that it is now stunned an any action in progress will be cancelled.
function CombatService:hit_stun(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   radiant.events.trigger_async(target, 'stonehearth:combat:hit_stun', context)
end

function CombatService:get_assault_events(target)
   if target == nil or not target:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(target)
   return combat_state:get_assault_events()
end

function CombatService:start_cooldown(entity, action_info)
   if entity == nil or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   combat_state:start_cooldown(action_info.name, action_info.cooldown)
end

function CombatService:in_cooldown(entity, action_name)
   if entity == nil or not entity:is_valid() then
      return false
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:in_cooldown(action_name)
end

function CombatService:get_cooldown_end_time(entity, action_name)
   if entity == nil or not entity:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:get_cooldown_end_time(action_name)
end

function CombatService:get_melee_weapon(entity)
   -- currently stored in a named slot on the equipment component. this might change.
   return radiant.entities.get_equipped_item(entity, 'melee_weapon')
end

function CombatService:get_melee_range(attacker, weapon_data, target)
   local attacker_radius = self:get_entity_radius(attacker)
   local target_radius = self:get_entity_radius(target)
   local weapon_reach = weapon_data.reach
   local melee_range_ideal = attacker_radius + weapon_reach + target_radius
   local melee_range_max = melee_range_ideal + 2
   return melee_range_ideal, melee_range_max
end

function CombatService:get_engage_range(attacker, weapon_data, target)
   local melee_range_ideal = self:get_melee_range(attacker, weapon_data, target)
   local engage_range_ideal = melee_range_ideal + 3
   local engage_range_max = engage_range_ideal + 3
   return engage_range_ideal, engage_range_max
end

function CombatService:get_entity_radius(entity)
   if entity == nil or not entity:is_valid() then
      return 1
   end

   -- TODO: get a real value
   return 1
end

function CombatService:get_time_to_impact(action_info)
   -- should we subtract 1 off the active frame?
   return action_info.active_frame * MS_PER_FRAME
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

function CombatService:set_panicking(entity, is_panicking)
   if entity == nil or not entity:is_valid() then
      return
   end

   local combat_state = self:get_combat_state(entity)
   combat_state:set_panicking(is_panicking)
end

function CombatService:is_panicking(entity)
   if entity == nil or not entity:is_valid() then
      return false
   end

   local combat_state = self:get_combat_state(entity)
   return combat_state:is_panicking()
end

function CombatService:get_combat_state(entity)
   if not entity or not entity:is_valid() then
      return nil
   end

   local combat_state_component = entity:add_component('stonehearth:combat_state')
   return combat_state_component:get_combat_state()
end

function CombatService:get_action_types(entity, action_type)
   local action_types = radiant.entities.get_entity_data(entity, action_type)

   -- TODO: just sort once...
   table.sort(action_types,
      function (a, b)
         return a.priority > b.priority
      end
   )
   return action_types
end

-- placeholder logic for now
function CombatService:choose_action(entity, action_types)
   local combat_state = self:get_combat_state(entity)
   local candidates = {}
   local priority = nil

   for i, action_info in ipairs(action_types) do
      if not combat_state:in_cooldown(action_info.name) then
         if priority == nil then
            table.insert(candidates, action_info)
            priority = action_info.priority
         else
            -- add any other available actions at the same priority
            if action_info.priority == priority then
               table.insert(candidates, action_info)
            else
               -- lower priority action, nothing else to find
               break
            end
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
