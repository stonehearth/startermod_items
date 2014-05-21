local DynamicScenario = require 'services.server.dynamic_scenario.dynamic_scenario'
local log = radiant.log.create_logger('dynamic_scenario_service')
local rng = _radiant.csg.get_default_rng()

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.running_scenarios then
      self._sv.running_scenarios = {}
      self._sv.last_spawn_times = {}
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            for idx, sv in pairs(self._sv.running_scenarios) do
               local scenario_data = sv:get_data()
               local scenario = {
                  scenario = radiant.mods.load_script(scenario_data._scenario_script_path),
                  properties = {
                     script = scenario_data._scenario_script_path
                  }
               }
               self._sv.running_scenarios[idx] = self:_init_scenario(scenario, sv)
            end
            self.__saved_variables:mark_changed()
         end)
   end

   self._dm = stonehearth.dm
   self:_parse_scenario_index()
end


-- At this point, the DM wants to spawn a scenario of kind 'scenario_type'.  Look
-- through our list of scenarios for that type, and see if we find one that we
-- can spawn.
function DynamicScenarioService:try_spawn_scenario(scenario_type, pace_keepers)
   -- Don't spawn too many of the same kinds of scenario too often!
   if self._sv.last_spawn_times[scenario_type] and
      stonehearth.calendar:get_elapsed_time() - self._sv.last_spawn_times[scenario_type] < 3600 then
      return
   end

   local valid_scenarios = {}
   for _, scenario in pairs(self._scenarios[scenario_type]) do
      local valid_scenario = true

      -- Check each 'type' that the scenario implements, to ensure that it is
      -- valid to spawn that scenario.  For example, the DM might want to spawn
      -- a combat scenario, and foo might be a valid combat scenario, but it might
      -- also effect the 'wealth' pacer, which isn't ready to run yet.  Or, all
      -- pacing is satisfied, but the scenario we're looking at is also waiting
      -- for the combat strength of the player to reach a certain level.
      for implementing_type,_ in pairs(scenario.properties.scenario_types) do
         local pace_keeper = pace_keepers[implementing_type]
         if not pace_keeper:willing_to_spawn() or 
            not pace_keeper:can_spawn_scenario(scenario) then
            valid_scenario = false
            break
         end
      end
      local rarity = self:_rarity_to_value(scenario.properties.rarity)
      if valid_scenario and rng:get_int(1, rarity) == 1 then
         table.insert(valid_scenarios, scenario)
      end
   end

   if #valid_scenarios < 1 then
      return
   end

   local scenario_idx = rng:get_int(1, #valid_scenarios)

   log:spam('Spawning new %s scenario %s', scenario_type, valid_scenarios[scenario_idx].properties.name)
   local new_scenario = self:_init_scenario(valid_scenarios[scenario_idx], nil)
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
   return 1
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
   self._scenarios = {}

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local properties = nil

   for _, file in pairs(scenario_index.dynamic.scenarios) do
      properties = radiant.resources.load_json(file)
      for scenario_type,_ in pairs(properties.scenario_types) do
         if self._scenarios[scenario_type] == nil then
            self._scenarios[scenario_type] = {}
         end
         local scenario_data = {
            scenario = radiant.mods.load_script(properties.script),
            properties = properties
         }
         table.insert(self._scenarios[scenario_type], scenario_data)
      end
   end
end


function DynamicScenarioService:_init_scenario(scenario, opt_datastore)
   local datastore = opt_datastore and opt_datastore or radiant.create_datastore()
   local dyn_scenario = DynamicScenario(scenario.scenario, scenario.properties.script, datastore)

   return dyn_scenario
end


return DynamicScenarioService
