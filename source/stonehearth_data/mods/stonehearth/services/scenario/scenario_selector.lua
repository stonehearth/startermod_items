local HabitatType = require 'services.world_generation.habitat_type'

local ScenarioSelector = class()
local log = radiant.log.create_logger('scenario_service')

-- frequency - The probability or expected number of scenarios in the category
--             that will (attempt to) be placed in a tile. Frequencies > 1.00 are supported.
-- priority - The order in which the scenarios will be placed (lower priorites may not find a site).
function ScenarioSelector:__init(frequency, priority, rng)
   self.frequency = frequency
   self.priority = priority
   self._rng = rng
   self._scenarios = {}

   -- these are only valid after calling _calculate_statistics
   self._num_scenarios = 0
   self._total_weight = 0
end

function ScenarioSelector:add(scenario)
   self._scenarios[scenario.name] = scenario
end

function ScenarioSelector:get(name)
   return self._scenarios[name]
end

function ScenarioSelector:remove(name)
   self._scenarios[name] = nil
end

function ScenarioSelector:select_scenarios(habitat_map)
   local rng = self._rng
   local frequency = self.frequency
   local scenario
   local selected_scenarios = {}

   -- recalculate statistics in case the available scenarios (or their weights) has changed
   -- e.g. a unique scenario was removed
   self:_calculate_statistics()

   if self._num_scenarios > 0 then
      while frequency > 0 do
         if rng:get_real(0, 1) < frequency then
            scenario = self:_pick_scenario()
            if scenario ~= nil then
               table.insert(selected_scenarios, scenario)
            end
         end
         frequency = frequency - 1.00
      end
   end

   return selected_scenarios
end

-- Pick a random scenario based on their weighted probabilities.
-- This is O(n) on the number of scenarios. Could be O(1) if necessary.
function ScenarioSelector:_pick_scenario()
   local roll = self._rng:get_int(1, self._total_weight)
   local sum = 0

   for key, value in pairs(self._scenarios) do
      sum = sum + value.weight
      if roll <= sum then
         return value
      end
   end

   log:error('Total weight in ScenarioSelector is incorrect!')
   return nil
end

function ScenarioSelector:_calculate_statistics()
   local count = 0
   local sum = 0

   for _, value in pairs(self._scenarios) do
      count = count + 1
      sum = sum + value.weight
   end

   self._num_scenarios = count
   self._total_weight = sum
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
