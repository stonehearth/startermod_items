local ScenarioSelector = require 'services.server.world_generation.scenario_selector'
local log = radiant.log.create_logger('surface_scenario_selector')

local SurfaceScenarioSelector = class()

function SurfaceScenarioSelector:__init(scenario_index, feature_size, rng)
   self._feature_size = feature_size
   self._rng = rng
   self._scenario_index = scenario_index
end

function SurfaceScenarioSelector:place_immediate_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local scenarios = self._scenario_index:select_scenarios('surface', 'immediate')
   self:_place_scenarios(scenarios, habitat_map, elevation_map, tile_offset_x, tile_offset_y, true)
end

function SurfaceScenarioSelector:place_revealed_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local scenarios = self._scenario_index:select_scenarios('surface', 'revealed')
   self:_place_scenarios(scenarios, habitat_map, elevation_map, tile_offset_x, tile_offset_y, false)
end

function SurfaceScenarioSelector:_place_scenarios(scenarios, habitat_map, elevation_map, tile_offset_x, tile_offset_y, activate_now)
   local rng = self._rng
   local feature_size = self._feature_size
   local feature_width, feature_length
   local feature_offset_x, feature_offset_y, intra_cell_offset_x, intra_cell_offset_y
   local site, sites, num_sites, roll, habitat_types, residual_x, residual_y
   local x, y, width, length

   for _, properties in pairs(scenarios) do
      habitat_types = properties.habitat_types

      -- dimensions of the scenario in voxels
      width = properties.size.width
      length = properties.size.length

      -- get dimensions of the scenario in feature cells
      feature_width, feature_length = self:_get_dimensions_in_feature_units(width, length)

      -- get a list of valid locations
      sites, num_sites = self:_find_valid_sites(habitat_map, elevation_map, habitat_types, feature_width, feature_length)

      if num_sites > 0 then
         -- pick a random location
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         feature_offset_x = (site.i-1)*feature_size
         feature_offset_y = (site.j-1)*feature_size

         residual_x = feature_width*feature_size - width
         residual_y = feature_length*feature_size - length

         intra_cell_offset_x = rng:get_int(0, residual_x)
         intra_cell_offset_y = rng:get_int(0, residual_y)

         -- these are in C++ base 0 array coordinates
         x = tile_offset_x + feature_offset_x + intra_cell_offset_x
         y = tile_offset_y + feature_offset_y + intra_cell_offset_y

         stonehearth.static_scenario:add_scenario(properties, x, y, width, length, activate_now)

         self:_mark_habitat_map(habitat_map, site.i, site.j, feature_width, feature_length)

         if properties.unique then
            self._scenario_index:remove_scenario(properties)
         end
      end
   end
end

function SurfaceScenarioSelector:_find_valid_sites(habitat_map, elevation_map, habitat_types, width, length)
   local i, j, is_habitat_type, is_flat, elevation
   local sites = {}
   local num_sites = 0

   local is_target_habitat_type = function(value)
      local found = habitat_types[value]
      return found
   end

   for j=1, habitat_map.height-(length-1) do
      for i=1, habitat_map.width-(width-1) do
         -- check if block meets habitat requirements
         is_habitat_type = habitat_map:visit_block(i, j, width, length, is_target_habitat_type)

         if is_habitat_type then
            -- check if block is flat
            elevation = elevation_map:get(i, j)

            is_flat = elevation_map:visit_block(i, j, width, length, function(value)
                  return value == elevation
               end)

            if is_flat then
               num_sites = num_sites + 1
               sites[num_sites] = { i = i, j = j}
            end
         end
      end
   end

   return sites, num_sites
end

function SurfaceScenarioSelector:_mark_habitat_map(habitat_map, i, j, width, length)
   habitat_map:set_block(i, j, width, length, 'occupied')
end

function SurfaceScenarioSelector:_get_dimensions_in_feature_units(width, length)
   local feature_size = self._feature_size
   return math.ceil(width/feature_size), math.ceil(length/feature_size)
end

return SurfaceScenarioSelector
