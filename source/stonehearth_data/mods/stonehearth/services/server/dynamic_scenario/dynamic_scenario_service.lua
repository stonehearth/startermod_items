local DynamicScenario = require 'services.server.dynamic_scenario.dynamic_scenario'
local log = radiant.log.create_logger('dynamic_scenario_service')
local rng = _radiant.csg.get_default_rng()

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()

   --[[self.__saved_variables:read_data(function(sv)
         self._rng = sv._rng
         self._revealed_region = sv._revealed_region
         self._dormant_scenarios = sv._dormant_scenarios
         self._feature_size = sv._feature_size
      end);]]

   self._dm = stonehearth.dm
   self._running_scenarios = {}
   
   --[[if self._rng then
      radiant.events.listen(radiant, 'radiant:game_loaded', function()
            self:_register_events()
            return radiant.events.UNLISTEN
         end)
   end]]
end


function DynamicScenarioService:spawn_scenario(scenario_kind, scenario_difficulty_min, scenario_difficulty_max)
   local scenarios = self._scenarios[scenario_kind]

   if scenarios == nil then
      return
   end

   local valid_scenarios = {}
   for _, scenario in pairs(scenarios) do
      if scenario.difficulty <= scenario_difficulty_max and scenario.difficulty >= scenario_difficulty_min then
         table.insert(valid_scenarios, scenario)
      end
   end

   if #valid_scenarios < 1 then
      return
   end

   local scenario_idx = rng:get_int(1, #valid_scenarios)

   local new_scenario = self:_start_scenario(valid_scenarios[scenario_idx])
   table.insert(self._running_scenarios, new_scenario)
end


function DynamicScenarioService:num_running_scenarios()
   local num_running = 0

   for i = #self._running_scenarios, 1, -1 do
      local scenario = self._running_scenarios[i]
      if scenario:is_running() then
         num_running = num_running + 1
      else 
         table.remove(self._running_scenarios, i)
      end
   end

   return num_running
end


function DynamicScenarioService:create_new_game()
   self:_parse_scenario_index()
end


function DynamicScenarioService:_parse_scenario_index()
   self._scenarios = {}

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local properties = nil

   for _, file in pairs(scenario_index.dynamic.scenarios) do
      properties = radiant.resources.load_json(file)
      if self._scenarios[properties.category] == nil then
         self._scenarios[properties.category] = {}
      end

      table.insert(self._scenarios[properties.category], properties)
   end
end


function DynamicScenarioService:_start_scenario(properties)
   local scenario_script = radiant.mods.load_script(properties.script)
   local dyn_scenario = DynamicScenario(scenario_script)
   dyn_scenario:start()

   return dyn_scenario
end

return DynamicScenarioService
