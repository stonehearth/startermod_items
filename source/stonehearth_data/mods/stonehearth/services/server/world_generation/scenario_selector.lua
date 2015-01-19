local WeightedSet = require 'lib.algorithms.weighted_set'
local log = radiant.log.create_logger('scenario_service')

local ScenarioSelector = class()

function ScenarioSelector:__init(terrain_info, rng)
   self._terrain_info = terrain_info
   self._rng = rng
   self._scenarios = {}
end

function ScenarioSelector:add(properties)
   self._scenarios[properties.name] = properties
end

function ScenarioSelector:remove(name)
   self._scenarios[properties.name] = nil
end

function ScenarioSelector:select_scenarios(density, habitat_volumes)
   local rng = self._rng
   local selected = {}

   for habitat_type, volume in pairs(habitat_volumes) do
      local candidates = WeightedSet(self._rng)

      -- Add any scenarios that can be placed in this habitat_type to the list of candidates
      for name, properties in pairs(self._scenarios) do
         if properties.habitat_types[habitat_type] then
            -- Weight scenarios by their volume so that a density of 1.0 means every cell is occupied.
            -- e.g. a 10x10 scenario takes up the same volume as 100 1x1 scenarios. For their densities
            -- to be equal, the 10x10 scenario should be selected 100x less frequently.
            local volume = self:_get_scenario_volume(properties)
            local effective_weight = properties.weight / volume
            candidates:add(properties, effective_weight)
         end
      end

      if candidates:get_total_weight() > 0 then
         -- Note that num is usually fractional
         local num = volume * density
         while num > 0 do
            local choose = num >= 1 or rng:get_real(0, 1) < num
            if choose then
               local properties = candidates:choose_random()
               table.insert(selected, properties)
            end
            num = num - 1
         end
      end
   end

   return selected
end

function ScenarioSelector:_get_scenario_volume(properties)
   local feature_width, feature_length = self._terrain_info:get_feature_dimensions(properties.size.width, properties.size.length)
   local feature_height = 1 -- TODO
   local volume = feature_width * feature_length * feature_height
   return volume
end

return ScenarioSelector
