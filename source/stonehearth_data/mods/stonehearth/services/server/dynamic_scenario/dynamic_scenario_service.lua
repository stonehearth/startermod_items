local log = radiant.log.create_logger('dynamic_scenario_service')

local DynamicScenarioService = class()

function DynamicScenarioService:initialize()

   --[[self.__saved_variables:read_data(function(sv)
         self._rng = sv._rng
         self._revealed_region = sv._revealed_region
         self._dormant_scenarios = sv._dormant_scenarios
         self._feature_size = sv._feature_size
      end);]]

   self._dm = stonehearth.dm
   
   
   --[[if self._rng then
      radiant.events.listen(radiant, 'radiant:game_loaded', function()
            self:_register_events()
            return radiant.events.UNLISTEN
         end)
   end]]
end

function DynamicScenarioService:create_new_game()
   self:_parse_scenario_index()
end

function DynamicScenarioService:_parse_scenario_index()
   self._scenarios = {}
   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local properties

   for _, file in pairs(scenario_index.dynamic.scenarios) do
      properties = radiant.resources.load_json(file)

      table.insert(self._scenarios, self:_activate_scenario(properties))

   end
end

function DynamicScenarioService:_activate_scenario(properties)
   local scenario_script

   scenario_script = radiant.mods.load_script(properties.script)
   scenario_script()
   scenario_script:initialize(properties)

   return scenario_script
end

return DynamicScenarioService
