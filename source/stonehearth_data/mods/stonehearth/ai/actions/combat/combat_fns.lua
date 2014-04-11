local CombatFns = class()

local MS_PER_FRAME = 1000/30

local combat_actions = {}

--stashing reusable functions here until we decide where to put them

function CombatFns.get_weapon_table(weapon_class)
   local file_name = string.format('/stonehearth/entities/weapons/%s.json', weapon_class)
   local weapon_table = radiant.resources.load_json(file_name)
   return weapon_table
end

function CombatFns.get_action_types(weapon_table, action_type)
   local action_types = {}
   local num_action_types = 0

   for name in pairs(weapon_table[action_type]) do
      table.insert(action_types, name)
      num_action_types = num_action_types + 1
   end

   return action_types, num_action_types
end

function CombatFns.get_impact_time(weapon_table, action_type, action_name)
   local active_frame = weapon_table[action_type][action_name].active_frame
   return active_frame * MS_PER_FRAME
end

function CombatFns.get_melee_range(attacker, weapon_class, target)
   local attacker_radius = CombatFns.get_entity_radius(attacker)
   local target_radius = CombatFns.get_entity_radius(target)
   local weapon_length = CombatFns.get_weapon_length(weapon_class)
   return attacker_radius + weapon_length + target_radius
end

function CombatFns.get_entity_radius(entity)
   -- TODO: get a real value
   return 0.6
end

function CombatFns.get_weapon_length(weapon_class)
   -- TODO: get a real value
   return 1
end

-- for testing only
function CombatFns.is_baddie(entity)
   local faction = entity:add_component('unit_info'):get_faction()
   return faction ~= 'civ'
end

function CombatFns.register_combat_action(entity, action)
   if entity and entity:is_valid() then
      combat_actions[entity:get_id()] = action
   end
end

function CombatFns.get_combat_action(entity)
   if entity and entity:is_valid() then
      return combat_actions[entity:get_id()]
   end
end

return CombatFns
