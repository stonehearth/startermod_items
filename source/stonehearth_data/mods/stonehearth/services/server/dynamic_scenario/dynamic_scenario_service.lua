local DynamicScenario = require 'services.server.dynamic_scenario.dynamic_scenario'
local log = radiant.log.create_logger('dynamic_scenario_service')
local rng = _radiant.csg.get_default_rng()

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.running_scenarios then
      self._sv.running_scenarios = {}
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            for idx, sv in pairs(self._sv.running_scenarios) do
               local scenario_data = sv:get_data()
               local properties = {
                  script = scenario_data._scenario_script_path
               }
               self._sv.running_scenarios[idx] = self:_init_scenario(properties, sv)
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
   local valid_scenarios = {}
   for _, scenario in pairs(self._scenarios[scenario_type]) do
      local valid_scenario = true

      -- Check each 'type' that the scenario implements, to ensure that it is
      -- valid to spawn that scenario.  For example, the DM might want to spawn
      -- a combat scenario, and foo might be a valid combat scenario, but it might
      -- also effect the 'wealth' pacer, which isn't ready to run yet.  Or, all
      -- pacing is satisfied, but the scenario we're looking at is also waiting
      -- for the combat strength of the player to reach a certain level.
      for _, implementing_type in pairs(scenario.scenario_types) do
         local pace_keeper = pace_keepers[implementing_type]
         if not pace_keeper:willing_to_spawn() or 
            not pace_keeper:can_spawn_scenario(scenario) then
            valid_scenario = false
            break
         end
      end
      if valid_scenario then
         table.insert(valid_scenarios, valid_scenario)
      end
   end

   if #valid_scenarios < 1 then
      return
   end

   local scenario_idx = rng:get_int(1, #valid_scenarios)

   local new_scenario = self:_init_scenario(valid_scenarios[scenario_idx], nil)
   new_scenario:start()
   table.insert(self._sv.running_scenarios, new_scenario)
   self.__saved_variables:mark_changed()
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
      for _, scen_types in pairs(properties.scenario_types) do
         if self._scenarios[scen_types] == nil then
            self._scenarios[scen_types] = {}
         end
         table.insert(self._scenarios[scen_types], properties)
      end
   end
end


function DynamicScenarioService:_init_scenario(properties, opt_datastore)
   local scenario_script = radiant.mods.load_script(properties.script)
   local datastore = opt_datastore and opt_datastore or radiant.create_datastore()
   local dyn_scenario = DynamicScenario(scenario_script, properties.script, datastore)

   return dyn_scenario
end


return DynamicScenarioService
