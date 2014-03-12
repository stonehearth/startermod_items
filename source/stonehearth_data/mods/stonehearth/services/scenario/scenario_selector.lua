local HabitatType = require 'services.world_generation.habitat_type'
local WeightedSet = require 'services.world_generation.math.weighted_set'

local ScenarioSelector = class()
local log = radiant.log.create_logger('scenario_service')

-- frequency - The probability or expected number of scenarios in the category
--             that will (attempt to) be placed in a tile. Frequencies > 1.00 are supported.
-- priority - The order in which the scenarios will be placed (lower priorites may not find a site).
function ScenarioSelector:__init(frequency, priority, rng)
   self.frequency = frequency
   self.priority = priority
   self._rng = rng
   self._scenarios = WeightedSet(self._rng)
end

function ScenarioSelector:add(name, weight)
   self._scenarios:add(name, weight)
end

function ScenarioSelector:remove(name)
   self._scenarios:remove(name)
end

function ScenarioSelector:select_scenarios(habitat_map)
   local rng = self._rng
   local frequency = self.frequency
   local scenario
   local selected_scenarios = {}

   while frequency > 0 do
      if rng:get_real(0, 1) < frequency then
         scenario = self._scenarios:choose_random()
         if scenario ~= nil then
            table.insert(selected_scenarios, scenario)
         end
      end
      frequency = frequency - 1.00
   end

   return selected_scenarios
end

function ScenarioSelector:_calculate_habitat_areas(habitat_map)
   local habitat_area = Histogram()

   habitat_map:visit(
      function (habitat_type)
         habitat_area:increment(habitat_type)
         return true
      end
   )
end

return ScenarioSelector
