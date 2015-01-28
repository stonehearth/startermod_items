local constants = require 'constants'
local Array2D = require 'services.server.world_generation.array_2D'
local ScenarioSelector = require 'services.server.world_generation.scenario_selector'
local WeightedSet = require 'lib.algorithms.weighted_set'
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('ore_scenario_selector')

local OreScenarioSelector = class()

function OreScenarioSelector:__init(scenario_index, terrain_info, rng)
   self._scenario_index = scenario_index
   self._terrain_info = terrain_info
   self._rng = rng
   self._y_cell_size = constants.mining.Y_CELL_SIZE
   self._min_elevation = self._terrain_info.mountains.step_size * 2
end

-- TODO: This code is heavily ore dependent. Generalize when we have other underground scenarios.
function OreScenarioSelector:place_revealed_scenarios(underground_elevation_map, elevation_map, tile_offset_x, tile_offset_y)
   local max_depth_in_slices = 6
   local weighted_set = WeightedSet(self._rng)

   underground_elevation_map:visit(function(elevation, i, j)
         local weight = math.floor((elevation - self._min_elevation) / self._y_cell_size)
         weight = math.min(weight, max_depth_in_slices)
         if weight > 0 then
            local index = { i = i, j = j }
            weighted_set:add(index, weight)
         end
      end)

   local habitat_volumes = {
      mountains = weighted_set:get_total_weight()
   }

   local placement_map = self:_create_placement_map(underground_elevation_map)
   local scenarios = self._scenario_index:select_scenarios('underground', 'revealed', habitat_volumes)
   local num_selected = #scenarios
   local num_placed = 0
   log:debug('%d scenarios selected', num_selected)

   -- because the ore scenarios are so big, the latter scenarios get squeezed out
   -- we randomize the order here so that this doesn't bias the distribution
   self:_randomize_scenarios(scenarios)

   for _, properties in pairs(scenarios) do
      -- allow scenarios to overlap tile boundaries for now
      local index = weighted_set:choose_random()
      if not index then
         break
      end

      local elevation = underground_elevation_map:get(index.i, index.j)
      local max_slice = self:_elevation_to_slice_index(elevation) - 1 -- -1 to get to max valid slice
      local min_slice = self:_elevation_to_slice_index(self._min_elevation)
      min_slice = math.max(min_slice, max_slice - (max_depth_in_slices-1)) -- -1 because inclusive range
      local slice_index = self._rng:get_int(min_slice, max_slice)

      -- location of the center of the mother lode
      local location = self:_calculate_location(tile_offset_x, tile_offset_y, index.i, index.j, slice_index)
      local context = { location = location }

      -- origin and size of the scenario in feature and world coordinates
      local i, j, feature_width, feature_length = self:_calculate_feature_params(index.i, index.j, properties)
      local x, y, width, length = self:_feature_to_world_space(tile_offset_x, tile_offset_y, i, j, feature_width, feature_length)

      if self:_is_unoccupied(placement_map, i, j, feature_width, feature_length, slice_index, slice_index) then
         stonehearth.static_scenario:add_scenario(properties, context, x, y, width, length)
         self:_mark_placement_map(placement_map, i, j, feature_width, feature_length, slice_index, slice_index)
         num_placed = num_placed + 1
      end
   end

   log:debug('%d scenarios placed', num_placed)   
end

function OreScenarioSelector:_elevation_to_slice_index(elevation)
   local slice_index = math.floor(elevation / self._y_cell_size)
   return slice_index
end

function OreScenarioSelector:_create_placement_map(underground_elevation_map)
   local width, height = underground_elevation_map:get_dimensions()
   local max_elevation = 0

   underground_elevation_map:visit(function(elevation)
         if elevation > max_elevation then
            max_elevation = elevation
         end
      end)

   local max_slice = self:_elevation_to_slice_index(max_elevation)
   local placement_map = {}
   for i=0, max_slice do
      placement_map[i] = Array2D(width, height)
   end

   return placement_map
end

function OreScenarioSelector:_mark_placement_map(placement_map, i, j, width, length, min_slice, max_slice)
   for k = min_slice, max_slice do
      placement_map[k]:set_block(i, j, width, length, true)
   end
end

function OreScenarioSelector:_is_unoccupied(placement_map, i, j, width, length, min_slice, max_slice)
   for k = min_slice, max_slice do
      local occupied = false

      placement_map[k]:visit_block(i, j, width, length, function(value)
            if value then
               occupied = true
               -- return true to terminate iteration
               return true
            end
         end)

      if occupied then
         return false
      end
   end

   return true
end

function OreScenarioSelector:_calculate_feature_params(i, j, properties)
   local feature_width, feature_length = self._terrain_info:get_feature_dimensions(properties.size.width, properties.size.length)
   local feature_x = i - math.floor(feature_width*0.5)
   local feature_y = j - math.floor(feature_length*0.5)
   return feature_x, feature_y, feature_width, feature_length
end

function OreScenarioSelector:_calculate_location(tile_offset_x, tile_offset_y, feature_index_x, feature_index_y, slice_index)
   local feature_size = self._terrain_info.feature_size
   local feature_middle = math.floor(feature_size*0.5)

   local x = tile_offset_x + (feature_index_x-1)*feature_size + feature_middle
   local y = slice_index * self._y_cell_size
   local z = tile_offset_y + (feature_index_y-1)*feature_size + feature_middle
   return Point3(x, y, z)
end

function OreScenarioSelector:_feature_to_world_space(tile_offset_x, tile_offset_y, i, j, feature_width, feature_length)
   local feature_size = self._terrain_info.feature_size

   local feature_offset_x = (i-1)*feature_size
   local feature_offset_y = (j-1)*feature_size

   local x = tile_offset_x + feature_offset_x
   local y = tile_offset_y + feature_offset_y
   local width = feature_width*feature_size
   local length = feature_length*feature_size

   return x, y, width, length
end

function OreScenarioSelector:_randomize_scenarios(scenarios)
   local num = #scenarios

   for i = 1, num-1 do
      local k = self._rng:get_int(i, num)
      scenarios[i], scenarios[k] = scenarios[k], scenarios[i]
   end
end

return OreScenarioSelector
