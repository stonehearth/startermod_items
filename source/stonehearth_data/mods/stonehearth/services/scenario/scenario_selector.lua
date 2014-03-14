local HabitatType = require 'services.world_generation.habitat_type'
local WeightedSet = require 'services.world_generation.math.weighted_set'
local Histogram = require 'services.world_generation.math.histogram'

local ScenarioSelector = class()
local log = radiant.log.create_logger('scenario_service')

-- frequency - The probability or expected number of scenarios in the category
--             that will (attempt to) be placed in a tile. Frequencies > 1.00 are supported.
-- priority - The order in which the scenarios will be placed (lower priorites may not find a site).
function ScenarioSelector:__init(frequency, priority, rng)
   self.frequency = frequency
   self.priority = priority
   self._rng = rng
   self._scenario_weights = WeightedSet(self._rng)
   self._scenario_properties = {}
end

function ScenarioSelector:add(properties)
   self._scenario_weights:add(properties.name, properties.weight)
   self._scenario_properties[properties.name] = properties
end

function ScenarioSelector:remove(name)
   self._scenario_weights:remove(properties.name)
   self._scenario_properties[properties.name] = nil
end

function ScenarioSelector:select_scenarios(habitat_map)
   local rng = self._rng
   local frequency = self.frequency
   local name, properties
   local selected_scenarios = {}

   while frequency > 0 do
      if rng:get_real(0, 1) < frequency then
         name = self._scenario_weights:choose_random()
         if name ~= nil then
            properties = self._scenario_properties[name]
            table.insert(selected_scenarios, properties)
         end
      end
      frequency = frequency - 1.00
   end

   return selected_scenarios
end

function ScenarioSelector:_calculate_habitat_areas(habitat_map)
   local habitat_areas = Histogram()

   habitat_map:visit(
      function (habitat_type)
         habitat_area:increment(habitat_type)
         return true
      end
   )

   return habitat_areas
end

return ScenarioSelector
