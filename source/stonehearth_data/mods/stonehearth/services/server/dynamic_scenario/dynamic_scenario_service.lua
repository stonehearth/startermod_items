local DynamicScenario = require 'services.server.dynamic_scenario.dynamic_scenario'
local log = radiant.log.create_logger('dynamic_scenario_service')
local rng = _radiant.csg.get_default_rng()

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.running_scenarios then
      self._sv.running_scenarios = {}
      self._sv.last_spawn_times = {}
      self._sv._scenarios = {}
      self:_parse_scenario_index()
   end
end


function DynamicScenarioService:force_spawn_scenario(scenario_name)
   for _, scenario_sets in pairs(self._sv._scenarios) do
      for _, scenario in pairs(scenario_sets) do
         if scenario.properties.name == scenario_name then
            local new_scenario = self:_create_scenario(scenario)
            new_scenario:start()
            table.insert(self._sv.running_scenarios, new_scenario)
            return new_scenario
         end
      end
   end
end


-- At this point, the DM wants to spawn a scenario of kind 'scenario_type'.  Look
-- through our list of scenarios for that type, and see if we find one that we
-- can spawn.
function DynamicScenarioService:try_spawn_scenario(scenario_type, pace_keepers)
   if not self._sv._scenarios[scenario_type] then
      return
   end

   local valid_scenarios = {}
   for _, scenario in pairs(self._sv._scenarios[scenario_type]) do
      local valid_scenario = true

      -- Check each 'type' that the scenario implements, to ensure that it is
      -- valid to spawn that scenario.  For example, the DM might want to spawn
      -- a combat scenario, and foo might be a valid combat scenario, but it might
      -- also effect the 'wealth' pacer, which isn't ready to run yet.  Or, all
      -- pacing is satisfied, but the scenario we're looking at is also waiting
      -- for the combat strength of the player to reach a certain level.
      for implementing_type,_ in pairs(scenario.properties.scenario_types) do
         local pace_keeper = pace_keepers[implementing_type]
         local temp_scenario = self:_create_scenario(scenario)

         if not pace_keeper:willing_to_spawn() or 
            not pace_keeper:is_valid_scenario(scenario) or
            not temp_scenario:can_spawn() then
            valid_scenario = false
            break
         end
         temp_scenario = nil
      end

      if valid_scenario then
         local rarity = self:_rarity_to_value(scenario.properties.rarity)
         if rng:get_int(1, rarity) == 1 then
            table.insert(valid_scenarios, scenario)
         else
            scenario.buildup = scenario.buildup + 1
         end
      end
   end

   if #valid_scenarios < 1 then
      return
   end

   -- We now have a list of valid scenarios that have also been randomly chosen according
   -- to their own rarities.  Now, just pick the one that has been used the least.
   local best_scenario = valid_scenarios[1]
   for _, scenario in pairs(valid_scenarios) do
      if best_scenario.buildup < scenario.buildup then
         best_scenario = scenario
      end
   end
   best_scenario.buildup = 0

   -- For each pace-keeper affected by this scenario, let that scenario know that we've spawned
   -- a scenario of that type.
   for implementing_type,_ in pairs(best_scenario.properties.scenario_types) do
      pace_keepers[implementing_type]:clear_buildup()
      pace_keepers[implementing_type]:spawning_scenario(best_scenario)
   end

   log:spam('Spawning new %s scenario %s', scenario_type, best_scenario.properties.name)
   
   local new_scenario = self:_create_scenario(best_scenario)
   new_scenario:start()
   table.insert(self._sv.running_scenarios, new_scenario)

   self.__saved_variables:mark_changed()
end


function DynamicScenarioService:_rarity_to_value(rarity)
   if rarity == 'common' then
      return 2 -- 50% chance of not being culled.
   elseif rarity == 'uncommon' then
      return 5 -- 20% chance of not being culled.
   elseif rarity == 'rare' then
      return 20 -- 5% chance of not being culled.
   end
   assert(false)
end


function DynamicScenarioService:num_running_scenarios()
   local num_running = 0

   for i = #self._sv.running_scenarios, 1, -1 do
      local scenario = self._sv.running_scenarios[i]
      if scenario:is_running() then
         num_running = num_running + 1
      else 
         table.remove(self._sv.running_scenarios, i)
      end
   end
   self.__saved_variables:mark_changed()

   return num_running
end


function DynamicScenarioService:create_new_game()
end


function DynamicScenarioService:_parse_scenario_index()
   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')

   for _, file in pairs(scenario_index.dynamic.scenarios) do
      local properties = radiant.resources.load_json(file)
      for scenario_type,_ in pairs(properties.scenario_types) do
         if self._sv._scenarios[scenario_type] == nil then
            self._sv._scenarios[scenario_type] = {}
         end
         local scenario_data = {
            uri = properties.uri,
            properties = properties,
            buildup = 0
         }
         table.insert(self._sv._scenarios[scenario_type], scenario_data)
      end
   end
end


function DynamicScenarioService:_create_scenario(scenario)
   local actual_scenario = radiant.create_controller(scenario.uri)
   local dyn_scenario = radiant.create_controller('stonehearth:dynamic_scenario', actual_scenario)

   return dyn_scenario
end


return DynamicScenarioService
