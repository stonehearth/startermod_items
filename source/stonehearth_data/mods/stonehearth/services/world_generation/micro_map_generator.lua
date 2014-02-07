local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local NonUniformQuantizer = require 'services.world_generation.math.non_uniform_quantizer'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local Timer = require 'services.world_generation.timer'

local MicroMapGenerator = class()
local log = radiant.log.create_logger('world_generation')

-- TODO: remove terrain_type from all the micro_maps
function MicroMapGenerator:__init(terrain_info, rng)
   self._terrain_info = terrain_info
   self._tile_size = self._terrain_info.tile_size
   self._macro_block_size = self._terrain_info.macro_block_size
   self._feature_size = self._terrain_info.feature_size
   self._rng = rng

   self._macro_blocks_per_tile = self._tile_size / self._macro_block_size
end

function MicroMapGenerator:generate_micro_map(blueprint)
   local quantizer = self._terrain_info.quantizer
   local noise_map, micro_map, width, height

   -- create the noise map
   noise_map = self:generate_noise_map(blueprint)
   width, height = noise_map:get_dimensions()
   micro_map = Array2D(width, height)

   -- pass it through a low pass filter
   FilterFns.filter_2D_0125(micro_map, noise_map, width, height, 10)

   -- quantize it
   micro_map:process(
      function (value)
         return quantizer:quantize(value)
      end
   )

   -- postprocess it
   self:_postprocess_micro_map(micro_map)

   local elevation_map = self:_convert_to_elevation_map(micro_map)

   return micro_map, elevation_map
end

function MicroMapGenerator:generate_noise_map(blueprint)
   local terrain_info = self._terrain_info
   local rng = self._rng
   local macro_blocks_per_tile = self._macro_blocks_per_tile
   -- +1 for half macro_block margin on each edge
   local width = blueprint.width * macro_blocks_per_tile + 1
   local height = blueprint.height * macro_blocks_per_tile + 1
   local noise_map = Array2D(width, height)
   local terrain_type, mean, std_dev, fn

   for j=1, blueprint.height do
      for i=1, blueprint.width do
         terrain_type = blueprint:get(i, j).terrain_type
         mean = terrain_info[terrain_type].mean_height
         std_dev = terrain_info[terrain_type].std_dev

         if terrain_type == TerrainType.mountains then
            if self:_surrounded_by_terrain(TerrainType.mountains, blueprint, i, j) then
               mean = mean + terrain_info[TerrainType.mountains].step_size
            end
         elseif terrain_type == TerrainType.foothills then
            if self:_surrounded_by_terrain(TerrainType.foothills, blueprint, i, j) then
               std_dev = std_dev * 1.20
            end
         else -- terrain_type == TerrainType.plains
            -- if self:_surrounded_by_terrain(TerrainType.plains, blueprint, i, j) then
            -- end
         end

         fn = function ()
            return rng:get_gaussian(mean, std_dev)
         end

         -- width and height are +1 to account for tile overlap
         noise_map:process_block((i-1)*macro_blocks_per_tile+1, (j-1)*macro_blocks_per_tile+1,
            macro_blocks_per_tile+1, macro_blocks_per_tile+1, fn)
      end
   end

   return noise_map
end

function MicroMapGenerator:_convert_to_elevation_map(micro_map)
   -- subtract the half macroblock margin from each edge
   local elevation_map_width = (micro_map.width-1)*2
   local elevation_map_height = (micro_map.height-1)*2
   local elevation_map = Array2D(elevation_map_width, elevation_map_height)
   local a, b, elevation

   for j=1, elevation_map.height do
      for i=1, elevation_map.width do
         a, b = _elevation_map_to_micro_map_coords(i, j)
         elevation = micro_map:get(a, b)
         elevation_map:set(i, j, elevation)
      end
   end

   return elevation_map
end

function _elevation_map_to_micro_map_coords(i, j)
   return _elevation_map_to_micro_map_index(i), _elevation_map_to_micro_map_index(j)
end

-- recall that micro_maps are offset by half a macroblock for shared tiling boundaries
function _elevation_map_to_micro_map_index(i)
   -- offset 1 element to skip over micro_map overlap
   -- convert to 0 based array, scale, convert back to 1 based array
   return math.floor((i+1-1) * 0.5) + 1
end

function MicroMapGenerator:_surrounded_by_terrain(terrain_type, blueprint, x, y)
   local tile_info

   tile_info = self:_get_tile_info(blueprint, x-1, y)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then
      return false
   end

   tile_info = self:_get_tile_info(blueprint, x+1, y)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then
      return false
   end

   tile_info = self:_get_tile_info(blueprint, x, y-1)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then
      return false
   end

   tile_info = self:_get_tile_info(blueprint, x, y+1)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then
      return false
   end

   return true
end

-- edge values may not change values! they are shared with the adjacent tile
function MicroMapGenerator:_postprocess_micro_map(micro_map)
   self:_fill_holes(micro_map)
end

function MicroMapGenerator:_fill_holes(micro_map)
   local i, j

   for j=1, micro_map.height do
      for i=1, micro_map.width do
         -- no need for a separate destination map, can fill in place
         self:_fill_hole(micro_map, i, j)
      end
   end
end

function MicroMapGenerator:_fill_hole(height_map, x, y)
   local width = height_map.width
   local height = height_map.height
   local offset = height_map:get_offset(x, y)
   local value = height_map[offset]
   local new_value = MathFns.MAX_INT32
   local neighbor

   -- uncomment if edge macro_blocks should not be eligible
   -- if x == 1 or x == width then return end
   -- if y == 1 or y == height then return end

   if x > 1 then
      neighbor = height_map[offset-1]
      if neighbor <= value then return end
      if neighbor < new_value then new_value = neighbor end
   end
   if x < width then
      neighbor = height_map[offset+1]
      if neighbor <= value then return end
      if neighbor < new_value then new_value = neighbor end
   end
   if y > 1 then
      neighbor = height_map[offset-width]
      if neighbor <= value then return end
      if neighbor < new_value then new_value = neighbor end
   end
   if y < height then
      neighbor = height_map[offset+width]
      if neighbor <= value then return end
      if neighbor < new_value then new_value = neighbor end
   end

   height_map[offset] = new_value
end

function MicroMapGenerator:_consolidate_mountain_blocks(micro_map)
   local max_foothills_height = self._terrain_info[TerrainType.foothills].max_height
   local start_x = 1
   local start_y = 1
   local i, j, value

   for j=start_y, micro_map.height, 2 do
      for i=start_x, micro_map.width, 2 do
         value = self:_average_quad(micro_map, i, j)
         if value > max_foothills_height then
            self:_set_quad(micro_map, i, j, value)
         end
      end
   end
end

function MicroMapGenerator:_average_quad(micro_map, x, y)
   local offset = micro_map:get_offset(x, y)
   local width = micro_map.width
   local height = micro_map.height
   local sum, num_blocks

   sum = micro_map[offset]
   num_blocks = 1

   if x < width then
      sum = sum + micro_map[offset+1]
      num_blocks = num_blocks + 1
   end

   if y < height then
      sum = sum + micro_map[offset+width]
      num_blocks = num_blocks + 1
   end

   if x < width and y < height then
      sum = sum + micro_map[offset+width+1]
      num_blocks = num_blocks + 1
   end

   return sum / num_blocks
end

function MicroMapGenerator:_set_quad(micro_map, x, y, value)
   local offset = micro_map:get_offset(x, y)
   local width = micro_map.width
   local height = micro_map.height

   micro_map[offset] = value

   if x < width then 
      micro_map[offset+1] = value
   end

   if y < height then
      micro_map[offset+width] = value
   end

   if x < width and y < height then
      micro_map[offset+width+1] = value
   end
end

function MicroMapGenerator:_get_tile_info(blueprint, x, y)
   if blueprint:in_bounds(x, y) then
      return blueprint:get(x, y)
   end
   return nil
end

function MicroMapGenerator:_get_micro_map(blueprint, x, y)
   local tile_info = self:_get_tile_info(blueprint, x, y)
   if tile_info == nil or not tile_info.generated then
      return nil
   end
   return tile_info.micro_map
end

-- not used. saving in case we need it later
function MicroMapGenerator:_copy_forced_edge_values(micro_map, blueprint, x, y)
   if blueprint == nil then return end

   local width = micro_map.width
   local height = micro_map.height
   local adj_micro_map

   -- left tile
   adj_micro_map = self:_get_micro_map(blueprint, x-1, y)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, 1, width, 1, 1, height)
   end

   -- right tile
   adj_micro_map = self:_get_micro_map(blueprint, x+1, y)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, width, 1, 1, 1, 1, height)
   end

   -- top tile
   adj_micro_map = self:_get_micro_map(blueprint, x, y-1)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, 1, 1, height, width, 1)
   end

   -- bottom tile
   adj_micro_map = self:_get_micro_map(blueprint, x, y+1)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, height, 1, 1, width, 1)
   end
end

return MicroMapGenerator
