local constants = require 'constants'
local Array2D = require 'services.server.world_generation.array_2D'
local ScenarioSelector = require 'services.server.world_generation.scenario_selector'
local WeightedSet = require 'lib.algorithms.weighted_set'
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('underground_scenario_selector')

local UndergroundScenarioSelector = class()

function UndergroundScenarioSelector:__init(scenario_index, terrain_info, rng)
   self._scenario_index = scenario_index
   self._terrain_info = terrain_info
   self._rng = rng
   self._y_cell_size = constants.mining.Y_CELL_SIZE
   self._min_elevation = self._terrain_info.mountains.step_size + self._y_cell_size
end

function UndergroundScenarioSelector:place_revealed_scenarios(underground_elevation_map, tile_offset_x, tile_offset_y)
   if not radiant.util.get_config('enable_ore', false) then
      return
   end

   local weighted_set = WeightedSet(self._rng)
   local placement_map = self:_create_placement_map(underground_elevation_map)
   local scenarios = self._scenario_index:select_scenarios('underground', 'revealed')

   -- uniformly distribute scenarios in the 3d volume above min height
   underground_elevation_map:visit(function(elevation, i, j)
         local weight = math.floor((elevation - self._min_elevation) / self._y_cell_size)
         if weight > 0 then
            local index = { i = i, j = j }
            weighted_set:add(index, weight)
         end
      end)

   for _, properties in pairs(scenarios) do
      local index = weighted_set:choose_random()
      if not index then
         break
      end

      local elevation = underground_elevation_map:get(index.i, index.j)
      local min_slice = self:_elevation_to_slice_index(self._min_elevation)
      local max_slice = self:_elevation_to_slice_index(elevation)
      local slice_index = self._rng:get_int(min_slice, max_slice)
      local location = self:_calculate_location(tile_offset_x, tile_offset_y, index.i, index.j, slice_index)
      local context = { location = location }

      -- test code
      if self:_is_unoccupied(placement_map, index.i-4, index.j-4, 9, 9, slice_index, slice_index) then
         stonehearth.static_scenario:add_scenario(properties, context, location.x-64, location.y-64, 129, 129)
         self:_mark_placement_map(placement_map, index.i-4, index.j-4, 9, 9, slice_index, slice_index)
      end
   end
end

function UndergroundScenarioSelector:_calculate_location(tile_offset_x, tile_offset_y, feature_index_x, feature_index_y, slice_index)
   local feature_size = self._terrain_info.feature_size
   local feature_middle = math.floor(feature_size*0.5)

   local x = tile_offset_x + (feature_index_x-1)*feature_size + feature_middle
   local y = slice_index * self._y_cell_size
   local z = tile_offset_y + (feature_index_y-1)*feature_size + feature_middle
   return Point3(x, y, z)
end

function UndergroundScenarioSelector:_elevation_to_slice_index(elevation)
   local slice_index = math.floor(elevation / self._y_cell_size)
   return slice_index
end

function UndergroundScenarioSelector:_create_placement_map(underground_elevation_map)
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

function UndergroundScenarioSelector:_mark_placement_map(placement_map, i, j, width, length, min_slice, max_slice)
   for k = min_slice, max_slice do
      placement_map[k]:set_block(i, j, width, length, true)
   end
end

function UndergroundScenarioSelector:_is_unoccupied(placement_map, i, j, width, length, min_slice, max_slice)
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

return UndergroundScenarioSelector
