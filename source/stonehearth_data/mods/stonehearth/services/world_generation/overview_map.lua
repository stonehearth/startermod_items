local Array2D = require 'services.world_generation.array_2D'
local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local log = radiant.log.create_logger('world_generation')

local OverviewMap = class()

function OverviewMap:__init(terrain_info, landscaper, tile_size, macro_block_size, feature_size)
   self._terrain_info = terrain_info
   self._landscaper = landscaper

   self._tile_size = tile_size
   self._features_per_tile = tile_size / feature_size
   self._macro_blocks_per_tile = tile_size / macro_block_size
   assert(macro_block_size / feature_size == 2)

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

function OverviewMap:derive_overview_map(blueprint)
   local terrain_info = self._terrain_info
   -- -1 to remove half-macroblock offset from both ends
   local overview_width = blueprint.width * self._macro_blocks_per_tile - 1
   local overview_height = blueprint.height * self._macro_blocks_per_tile - 1
   local overview_map = Array2D(overview_width, overview_height)
   local a, b, terrain_code, forest_density, macro_block_info
   local full_feature_map, full_elevation_map

   full_feature_map, full_elevation_map = self:_assemble_maps(blueprint)

   for j=1, overview_width do
      for i=1, overview_height do
         a, b = _overview_map_to_feature_map_coords(i, j)
         terrain_code = self:_get_terrain_code(full_elevation_map, a, b)
         forest_density = self:_get_forest_density(full_feature_map, a, b)
         macro_block_info = {
            terrain_code = terrain_code,
            forest_density = forest_density
         }
         overview_map:set(i, j, macro_block_info)
      end
   end

   self._width = overview_width
   self._height = overview_height
   self._map = overview_map
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

function OverviewMap:_get_terrain_code(elevation_map, i, j)
   -- since macroblocks are 2x the feature size, all 4 cells have the same value
   -- if this changes, sort and return the 3rd item (one of the tied medians)
   local elevation = elevation_map:get(i, j)
   return self._terrain_info:get_terrain_code(elevation)
end

function OverviewMap:_get_forest_density(forest_map, i, j)
   local landscaper = self._landscaper
   local value
   local count = 0

   -- count the 4 feature blocks in this macro block
   forest_map:visit_block(i, j, 2, 2,
      function (value)
         if landscaper:is_forest_feature(value) then
            count = count + 1
         end
         return true
      end
   )

   return count
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
