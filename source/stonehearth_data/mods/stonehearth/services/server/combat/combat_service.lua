local CombatState = require 'services.server.combat.combat_state'
local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('combat')

CombatService = class()

local MS_PER_FRAME = 1000/30

function CombatService:__init()
   self._hit_stun_damage_threshold = radiant.util.get_config('hit_stun_damage_threshold', 0.10)
   self._combat_state_table = {}

   -- TODO: change this to minute (or ten second) poll
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._remove_expired_objects)
end

function CombatService:initialize()
   log:write(0, 'initialize not implemented for combat service!')
end

function CombatService:get_weapon_table(weapon_class)
   local file_name = string.format('/stonehearth/entities/weapons/%s.json', weapon_class)
   local weapon_table = radiant.resources.load_json(file_name)
   return weapon_table
end

function CombatService:get_action_types(weapon_table, action_type)
   local action_types = weapon_table[action_type]

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

function CombatService:get_melee_range(attacker, weapon_class, target)
   local attacker_radius = self:get_entity_radius(attacker)
   local target_radius = self:get_entity_radius(target)
   local weapon_length = self:get_weapon_length(weapon_class)
   return attacker_radius + weapon_length + target_radius
end

function CombatService:get_entity_radius(entity)
   if entity == nil or not entity:is_valid() then
      return 1
   end

   -- TODO: get a real value
   return 1
end

function CombatService:get_weapon_length(weapon_class)
   -- TODO: get a real value
   return 1
end

function CombatService:get_combat_state(entity)
   if entity == nil or not entity:is_valid() then
      return nil
   end

   local combat_state = self._combat_state_table[entity:get_id()]

   if combat_state == nil then
      combat_state = CombatState()
      self._combat_state_table[entity:get_id()] = combat_state
   end

   return combat_state
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

function CombatService:engage(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   radiant.events.trigger_async(target, 'stonehearth:combat:engage', context)
end

function CombatService:assault(target, context)
   if target == nil or not target:is_valid() then
      return nil
   end

   local combat_state = self:get_combat_state(target)
   combat_state:add_assault_event(context)

   radiant.events.trigger_async(target, 'stonehearth:combat:assault', context)
end

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
   local entity

   for id, combat_state in pairs(self._combat_state_table) do
      combat_state:_remove_expired_cooldowns()
      combat_state:_remove_expired_assault_events()

      -- remove combat_state if entity is no longer valid
      entity = radiant.entities.get_entity(id)
      if entity == nil or not entity:is_valid() then
         -- safe to remove entries in a pairs loop: http://lua-users.org/wiki/TablesTutorial
         self._combat_state_table[id] = nil
      end
   end
end

-- for testing only
function CombatService:is_baddie(entity)
   if entity == nil or not entity:is_valid() then
      return false
   end

   local faction = entity:add_component('unit_info'):get_faction()
   return faction ~= 'civ'
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
