local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local log = radiant.log.create_logger('world_generation')

local OverviewMap = class()

function OverviewMap:__init(terrain_info, landscaper)
   self._terrain_info = terrain_info
   self._landscaper = landscaper
   self._tile_size = self._terrain_info.tile_size
   self._macro_block_size = self._terrain_info.macro_block_size
   self._feature_size = self._terrain_info.feature_size

   self._margin = self._macro_block_size / 2
   self._features_per_tile = self._tile_size / self._feature_size
   self._macro_blocks_per_tile = self._tile_size / self._macro_block_size

   -- scoring parameters
   self._radius = 8
   self._num_quanta = 5

   self:clear()
end

function OverviewMap:get_dimensions()
   return self._width, self._height
end

function OverviewMap:get_map()
   return self._map
end

function OverviewMap:clear()
   self._width = nil
   self._height = nil
   self._map = nil
end

function OverviewMap:derive_overview_map(full_elevation_map, full_feature_map, origin_x, origin_y)
   -- -1 to remove half-macroblock offset from both ends
   local overview_width = full_elevation_map.width/2 - 1
   local overview_height = full_elevation_map.height/2 - 1
   local overview_map = Array2D(overview_width, overview_height)
   local terrain_info = self._terrain_info
   local a, b, terrain_code, forest_density, macro_block_info

   -- only needed if we need to reassemble the full map from the shards in the blueprint
   -- local full_feature_map, full_elevation_map
   -- full_feature_map, full_elevation_map = self:_assemble_maps(blueprint)

   for j=1, overview_width do
      for i=1, overview_height do
         a, b = _overview_map_to_feature_map_coords(i, j)

         macro_block_info = {
            terrain_code = self:_get_terrain_code(a, b, full_elevation_map),
            forest_density = self:_get_forest_density(a, b, full_feature_map),
            wildlife_density = self:_get_wildlife_density(a, b, full_elevation_map),
            vegetation_density = self:_get_vegetation_density(a, b, full_feature_map)
         }
         overview_map:set(i, j, macro_block_info)
      end
   end

   self._width = overview_width
   self._height = overview_height
   self._map = overview_map

   -- discard the half macro block offset on the outside
   local margin = self._macro_block_size/2
   -- location of the world origin in the coordinate system of the overview map
   self._origin_x = origin_x - margin
   self._origin_y = origin_y - margin
end

-- get the world coordinates for the center of feature cell at (i,j)
-- i, j are base 1
function OverviewMap:get_coords_of_cell_center(i, j)
   local macro_block_size = self._macro_block_size
   local center_offset = macro_block_size * 0.5

   local offset_x = (i-1)*macro_block_size - self._origin_x + center_offset
   local offset_y = (j-1)*macro_block_size - self._origin_y + center_offset

   return offset_x, offset_y
end

function OverviewMap:_assemble_maps(blueprint)
   local features_per_tile = self._features_per_tile
   local full_width = blueprint.width * features_per_tile
   local full_height = blueprint.height * features_per_tile
   local full_feature_map = Array2D(full_width, full_height)
   local full_elevation_map = Array2D(full_width, full_height)
   local tile_info, dst_x, dst_y

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         tile_info = blueprint:get(i, j)
         dst_x = (i-1)*features_per_tile+1
         dst_y = (j-1)*features_per_tile+1

         Array2D.copy_block(full_feature_map, tile_info.feature_map,
            dst_x, dst_y, 1, 1, features_per_tile, features_per_tile)

         Array2D.copy_block(full_elevation_map, tile_info.elevation_map,
            dst_x, dst_y, 1, 1, features_per_tile, features_per_tile)
      end
   end

   return full_feature_map, full_elevation_map
end

function OverviewMap:_get_terrain_code(i, j, elevation_map)
   -- since macroblocks are 2x the feature size, all 4 cells have the same value
   -- if this changes, sort and return the 3rd item (one of the tied medians)
   local elevation = elevation_map:get(i, j)
   return self._terrain_info:get_terrain_code(elevation)
end

function OverviewMap:_get_forest_density(i, j, feature_map)
   local landscaper = self._landscaper
   local value
   local count = 0

   -- count the 4 feature blocks in this macro block
   feature_map:visit_block(i, j, 2, 2,
      function (value)
         if landscaper:is_forest_feature(value) then
            count = count + 1
         end
         return true
      end
   )

   return count
end

-- placeholder density function
function OverviewMap:_get_wildlife_density(i, j, elevation_map)
   local terrain_info = self._terrain_info
   local num_quanta = self._num_quanta
   local radius = self._radius
   local length = 2*radius+1
   local start_a, start_b, width, height, elevation, terrain_type, score
   local sum = 0

   start_a, start_b, width, height = elevation_map:bound_block(i-radius, j-radius, length, length)

   -- iterate instead of using visit_block as we may want to incorporate feature_map later
   for b=start_b, start_b+height-1 do
      for a=start_a, start_a+width-1 do
         elevation = elevation_map:get(a, b)
         terrain_type = terrain_info:get_terrain_type(elevation)
         if terrain_type ~= TerrainType.mountains then
            -- basic plains and foothills are average density (0.5 out of 1)
            sum = sum + 0.5
         end
      end
   end

   score = MathFns.round(sum / (width*height) * (num_quanta-1))
   return score
end

function OverviewMap:_get_vegetation_density(i, j, feature_map)
   local landscaper = self._landscaper
   local num_quanta = self._num_quanta
   local radius = self._radius
   local length = 2*radius+1
   local start_a, start_b, width, height, score
   local sum = 0

   start_a, start_b, width, height = feature_map:bound_block(i-radius, j-radius, length, length)

   feature_map:visit_block(start_a, start_b, width, height,
      function (value)
         if landscaper:is_tree_name(value) then
            -- 1.25 to account for thinned out trees
            sum = sum + 1.25
         end
         return true
      end
   )

   score = MathFns.round(sum / (width*height) * (num_quanta-1))
   return score
end

function _overview_map_to_feature_map_coords(i, j)
   return _overview_map_to_feature_map_index(i), _overview_map_to_feature_map_index(j)
end

function _overview_map_to_feature_map_index(i)
   -- -1 to base 0
   -- *0.5 to scale
   -- +1 to base 1
   -- +1 to align to macroblock boundary (recall that tiles are offset/overlap by half macroblock)
   return math.floor((i-1)*2) + 1 + 1
end

return OverviewMap
