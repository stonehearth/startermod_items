local HabitatType = require 'services.world_generation.habitat_type'
local WeightedSet = require 'services.world_generation.math.weighted_set'
local Histogram = require 'services.world_generation.math.histogram'

local ScenarioSelector = class()
local log = radiant.log.create_logger('scenario_service')

-- frequency - The probability or expected number of scenarios in the category
--             that will (attempt to) be placed in a tile. Frequencies > 1.00 are supported.
-- priority - The order in which the scenarios will be placed (lower priorites may not find a site).
function ScenarioSelector:__init(frequency, priority, activation_type, rng)
   self.frequency = frequency
   self.priority = priority
   self.activation_type = activation_type
   self._rng = rng
   self._scenarios = {}
end

function ScenarioSelector:add(properties)
   self._scenarios[properties.name] = properties
end

function ScenarioSelector:remove(name)
   self._scenarios[properties.name] = nil
end

function ScenarioSelector:select_scenarios(habitat_map)
   local rng = self._rng
   local frequency = self.frequency
   local selected = {}
   local candidates = {}
   local num_candidates = 0
   local properties

   candidates = WeightedSet(self._rng)

   for _, properties in pairs(self._scenarios) do
      candidates:add(properties, properties.weight)
      num_candidates = num_candidates + 1
   end

   if num_candidates > 0 then
      while frequency > 0 do
         if rng:get_real(0, 1) < frequency then
            properties = candidates:choose_random()
            table.insert(selected, properties)
         end
         frequency = frequency - 1.00
      end
   end

   return selected
end

-- for future use when moving to a more transparent density metric
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
