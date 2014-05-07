local CombatState = require 'services.server.combat.combat_state'
local TargetTable = require 'services.server.combat.target_table'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

CombatService = class()

local MS_PER_FRAME = 1000/30

function CombatService:__init()
end

-- TODO: move target tables and combat state into components
function CombatService:initialize()
   -- always stun for now
   --self._hit_stun_damage_threshold = radiant.util.get_config('hit_stun_damage_threshold', 0.10)
   self._hit_stun_damage_threshold = 0

   self._sv = self.__saved_variables:get_data()
   if not self._sv._combat_state_table then
      self._sv._combat_state_table = {}
      self._sv._target_tables = {}
   end

   self._combat_state_table = self._sv._combat_state_table
   self._target_tables = self._sv._target_tables

   -- TODO: change this to minute (or ten second) poll
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._remove_expired_objects)

   log:write(0, 'initialize not tested for combat service!')
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

function CombatService:get_melee_range(attacker, weapon_data, target)
   local attacker_radius = self:get_entity_radius(attacker)
   local target_radius = self:get_entity_radius(target)
   local weapon_reach = weapon_data.reach
   return attacker_radius + weapon_reach + target_radius
end

function CombatService:get_entity_radius(entity)
   if entity == nil or not entity:is_valid() then
      return 1
   end

   -- TODO: get a real value
   return 1
end

function CombatService:get_combat_state(entity)
   if entity == nil or not entity:is_valid() then
      return nil
   end

   local entity_id = entity:get_id()
   local combat_state = self._combat_state_table[entity_id]

   if combat_state == nil then
      combat_state = CombatState()
      self._combat_state_table[entity_id] = combat_state
   end

   return combat_state
end

function CombatService:get_target_table(entity, table_name)
   if entity == nil or not entity:is_valid() then
      return nil
   end

   local entity_id = entity:get_id()
   local target_tables = self._target_tables[entity_id]

   if target_tables == nil then
      target_tables = {}
      self._target_tables[entity_id] = target_tables
   end

   local target_table = target_tables[table_name]

   if target_table == nil then
      target_table = TargetTable()
      target_tables[table_name] = target_table
   end

   return target_table
end

function CombatService:start_cooldown(entity, action_info)
   if entity == nil or not entity:is_valid() then
      return
   end

   local combat_state = self._combat_state_table[entity:get_id()]
   combat_state:start_cooldown(action_info.name, action_info.cooldown)
end

function CombatService:in_cooldown(entity, action_name)
   if entity == nil or not entity:is_valid() then
      return false
   end

   local combat_state = self._combat_state_table[entity:get_id()]
   return combat_state:in_cooldown(action_name)
end

function CombatService:get_cooldown_end_time(entity, action_name)
   if entity == nil or not entity:is_valid() then
      return nil
   end

   local combat_state = self._combat_state_table[entity:get_id()]
   return combat_state:get_cooldown_end_time(action_name)
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

-- Ntify target that an attack has begun and will impact soon.
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
         self:hit_stun(target, BatteryContext())
      end
   end

   health = health - damage
   if health ~= nil then
      attributes_component:set_attribute('health', health)
   end
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

function CombatService:_remove_expired_objects()
   self:_clean_combat_state_table()
   self:_clean_target_tables()
end

function CombatService:_clean_combat_state_table()
   local entity

   for id, combat_state in pairs(self._combat_state_table) do
      entity = radiant.entities.get_entity(id)

      if entity == nil or not entity:is_valid() then
         -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
         self._combat_state_table[id] = nil
      else
         combat_state:remove_expired_cooldowns()
         combat_state:remove_expired_assault_events()
      end
   end
end

function CombatService:_clean_target_tables()
   local entity

   for id in pairs(self._target_tables) do
      entity = radiant.entities.get_entity(id)

      if entity == nil or not entity:is_valid() then
         -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
         self._target_tables[id] = nil
      else
         local target_tables = self._target_tables[id]
         for _, target_table in pairs(target_tables) do
            target_table:remove_invalid_targets()
         end
      end
   end
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
