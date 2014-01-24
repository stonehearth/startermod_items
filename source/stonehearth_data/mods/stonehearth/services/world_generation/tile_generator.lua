local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local NonUniformQuantizer = require 'services.world_generation.math.non_uniform_quantizer'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local Wavelet = require 'services.world_generation.filter.wavelet'
local WaveletFns = require 'services.world_generation.filter.wavelet_fns'
local TerrainDetailer = require 'services.world_generation.terrain_detailer'
local Timer = require 'services.world_generation.timer'

local TerrainGenerator = class()
local log = radiant.log.create_logger('world_generation')

-- Definitions
-- Block = atomic unit of terrain that cannot be subdivided
-- MacroBlock = square unit of flat land, 32x32, but can shift a bit due to toplogy
-- Tile = 2D array of MacroBlocks fitting a theme (plains, foothills, mountains)
--        These 256x256 terrain tiles are different from nav grid tiles which are 16x16.
-- World = the entire playspace of a game

function TerrainGenerator:__init(terrain_info, tile_size, macro_block_size, rng, async)
   if async == nil then async = false end

   self._terrain_info = terrain_info
   self._tile_size = tile_size
   self._macro_block_size = macro_block_size
   self._rng = rng
   self._async = async

   self._wavelet_levels = 4
   self._frequency_scaling_coeff = 0.7

   local oversize_tile_size = self._tile_size + self._macro_block_size
   self._oversize_map_buffer = Array2D(oversize_tile_size, oversize_tile_size)

   self._terrain_detailer = TerrainDetailer(self._terrain_info, oversize_tile_size, oversize_tile_size, self._rng)
end

function TerrainGenerator:generate_tile(micro_map)
   local oversize_map = self._oversize_map_buffer
   local tile_map

   self:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   self:_yield()

   self:_shape_height_map(oversize_map, self._frequency_scaling_coeff, self._wavelet_levels)
   self:_yield()

   self:_quantize_height_map(oversize_map, false)
   self:_yield()

   self:_add_additional_details(oversize_map, micro_map)
   self:_yield()

   -- copy the offset tile map from the oversize map
   tile_map = self:_extract_tile_map(oversize_map)
   self:_yield()

   return tile_map
end

function TerrainGenerator:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   oversize_map.terrain_type = micro_map.terrain_type

   for j=1, micro_height do
      for i=1, micro_width do
         value = micro_map:get(i, j)
         oversize_map:set_block((i-1)*self._macro_block_size+1, (j-1)*self._macro_block_size+1,
            self._macro_block_size, self._macro_block_size, value)
      end
   end
end

function TerrainGenerator:_shape_height_map(height_map, freq_scaling_coeff, levels)
   local width = height_map.width
   local height = height_map.height

   Wavelet.DWT_2D(height_map, width, height, levels, self._async)
   WaveletFns.scale_high_freq(height_map, width, height, freq_scaling_coeff, levels)
   self:_yield()
   Wavelet.IDWT_2D(height_map, width, height, levels, self._async)
end

function TerrainGenerator:_quantize_height_map(height_map, is_micro_map)
   local quantizer = self._terrain_info.quantizer
   local enable_fancy_quantizer = true

   height_map:process(
      function(value)
         local quantized_value = quantizer:quantize(value)

         if not enable_fancy_quantizer then
            return quantized_value
         end

         local terrain_info = self._terrain_info
         -- disable fancy mode for plains
         if quantized_value <= terrain_info[TerrainType.plains].max_height then
            return quantized_value
         end

         -- fancy mode
         local terrain_type = terrain_info:get_terrain_type(value)
         local step_size = terrain_info[terrain_type].step_size
         local rounded_value = MathFns.round(value)
         local diff = quantized_value - rounded_value

         if diff == step_size*0.5 then
            return rounded_value
         else
            return quantized_value
         end
      end
   )
end

function TerrainGenerator:_add_additional_details(height_map, micro_map)
   self._terrain_detailer:remove_mountain_chunks(height_map, micro_map)

   self._terrain_detailer:add_detail_blocks(height_map)

   -- handle plains separately - don't screw up edge detailer code
   self._terrain_detailer:add_plains_details(height_map)
end

-- must return a new tile_map each time
function TerrainGenerator:_extract_tile_map(oversize_map)
   local tile_map_origin = self._macro_block_size/2 + 1
   local tile_map = Array2D(self._tile_size, self._tile_size)

   tile_map.terrain_type = oversize_map.terrain_type

   Array2D.copy_block(tile_map, oversize_map,
      1, 1, tile_map_origin, tile_map_origin, self._tile_size, self._tile_size)

   return tile_map
end

function TerrainGenerator:_yield()
   if self._async then
      coroutine.yield()
   end
end

return TerrainGenerator
