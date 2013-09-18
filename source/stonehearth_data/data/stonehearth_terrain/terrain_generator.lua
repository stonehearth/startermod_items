local TerrainGenerator = class()

local TerrainType = require 'terrain_type.lua'
local Array2D = require 'array_2D'
local HeightMap = require 'height_map'
local GaussianRandom = require 'math.gaussian_random'
local MathFns = require 'math.math_fns'
local FilterFns = require 'filter.filter_fns'
local Wavelet = require 'wavelet.wavelet'
local WaveletFns = require 'wavelet.wavelet_fns'
local EdgeDetailer = require 'edge_detailer'
local Timer = require 'timer.lua'

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (foothills, plains, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init()
   local terrain_type, terrain_info
   math.randomseed(3)

   self.zone_size = 256
   self.tile_size = 32

   self.wavelet_levels = 4
   self.frequency_scaling_coeff = 0.7

   local base_step_size = 8
   self.terrain_info = {}
   terrain_info = self.terrain_info

   terrain_type = TerrainType.Plains
   terrain_info[terrain_type] = {}
   terrain_info[terrain_type].step_size = 2
   terrain_info[terrain_type].mean_height = base_step_size
   terrain_info[terrain_type].std_dev = 10
   terrain_info[terrain_type].max_height = 8

   terrain_type = TerrainType.Foothills
   terrain_info[terrain_type] = {}
   terrain_info[terrain_type].step_size = base_step_size
   terrain_info[terrain_type].mean_height = 24
   terrain_info[terrain_type].std_dev = 32
   terrain_info[terrain_type].max_height = 32

   terrain_type = TerrainType.Mountains
   terrain_info[terrain_type] = {}
   terrain_info[terrain_type].step_size = base_step_size*2
   terrain_info[terrain_type].mean_height = 72
   terrain_info[terrain_type].std_dev = 96

   terrain_info.tree_line = terrain_info[TerrainType.Foothills].max_height +
      terrain_info[TerrainType.Mountains].step_size

   local oversize_zone_size = self.zone_size + self.tile_size
   self.oversize_map_buffer = HeightMap(oversize_zone_size, oversize_zone_size)

   local micro_size = oversize_zone_size / self.tile_size
   self.noise_map_buffer = HeightMap(micro_size, micro_size)

   self._edge_detailer = EdgeDetailer()
end

function TerrainGenerator:generate_zone(terrain_type, zones, x, y)
   local zone_map, micro_map, noise_map
   local noise_map = self.noise_map_buffer
   local oversize_map = self.oversize_map_buffer

   local total_timer = Timer(Timer.CPU_TIME)
   local subset_timer = Timer(Timer.CPU_TIME)
   total_timer:start()

   noise_map.terrain_type = terrain_type
   oversize_map.terrain_type = terrain_type

   self:_fill_noise_map(noise_map, zones, x, y)

   -- propagate edge information from surrounding zones into current map
   self:_copy_zone_context(noise_map, zones, x, y, true)

   -- micro_map must be a new HeightMap as it is returned from the function
   micro_map = self:_filter_and_quantize_noise_map(noise_map)

   -- force shared tiles to same value from adjacent zones
   self:_copy_zone_context(micro_map, zones, x, y, false)

   self:_postprocess_micro_map(micro_map)

   --radiant.log.info('micro_map %d, %d:', x, y)
   --micro_map:print()

   self:_create_macro_map_from_micro_map(oversize_map, micro_map)

   -- copy the offset zone map from the oversize map
   -- zone_map must be a new HeightMap as it is returned from the function
   zone_map = self:_extract_zone_map(oversize_map)
   
   subset_timer:start()
   self:_shape_height_map(zone_map)

   -- enable non-uniform quantizer within a terrain type
   -- generates additional edge details
   self:_quantize_height_map(zone_map, true)
   
   self._edge_detailer:add_detail_blocks(zone_map)
   subset_timer:stop()

   total_timer:stop()
   --radiant.log.info('Zone generation time: %f', total_timer:seconds())
   --radiant.log.info('Subset time: %f', subset_timer:seconds())
   --radiant.log.info('Percent subset: %f', subset_timer:seconds()/total_timer:seconds())

   return zone_map, micro_map
end

function TerrainGenerator:_fill_noise_map(noise_map, zones, x, y)
   local terrain_info = self.terrain_info
   local width = noise_map.width
   local height = noise_map.height
   local terrain_type = noise_map.terrain_type
   local std_dev = terrain_info[terrain_type].std_dev
   local mean_height = terrain_info[terrain_type].mean_height

   noise_map:process_map(
      function ()
         return GaussianRandom.generate(mean_height, std_dev)
      end
   )

   if zones == nil then return end

   local left_terrain_type   = self:_get_terrain_type(zones, x-1, y)
   local right_terrain_type  = self:_get_terrain_type(zones, x+1, y)
   local top_terrain_type    = self:_get_terrain_type(zones, x,   y-1)
   local bottom_terrain_type = self:_get_terrain_type(zones, x,   y+1)
   local left_mean, right_mean, top_mean, bottom_mean, corner_mean
   local i, offset, start, stop

   if left_terrain_type   then left_mean   = terrain_info[left_terrain_type].mean_height   end
   if right_terrain_type  then right_mean  = terrain_info[right_terrain_type].mean_height  end
   if top_terrain_type    then top_mean    = terrain_info[top_terrain_type].mean_height    end
   if bottom_terrain_type then bottom_mean = terrain_info[bottom_terrain_type].mean_height end

   -- walk the four edges, but handle corners with multiple neighbors later
   start, stop = self:_calc_edge_range(1, height, top_terrain_type, bottom_terrain_type)

   if left_terrain_type then
      for i=start, stop do
         noise_map:set(1, i, left_mean)
         --offset = noise_map:get_offset(1, i)
         --noise_map[offset] = noise_map[offset] - mean_height + left_mean
      end
   end

   if right_terrain_type then
      for i=start, stop do
         noise_map:set(width, i, right_mean)
         --offset = noise_map:get_offset(width, i)
         --noise_map[offset] = noise_map[offset] - mean_height + right_mean
      end
   end

   start, stop = self:_calc_edge_range(1, width, left_terrain_type, right_terrain_type)

   if top_terrain_type then
      for i=start, stop do
         noise_map:set(i, 1, top_mean)
         --offset = noise_map:get_offset(i, 1)
         --noise_map[offset] = noise_map[offset] - mean_height + top_mean
      end
   end

   if bottom_terrain_type then
      for i=start, stop do
         noise_map:set(i, height, bottom_mean)
         --offset = noise_map:get_offset(i, height)
         --noise_map[offset] = noise_map[offset] - mean_height + bottom_mean
      end
   end

   -- handle corners with multiple bordering zones
   local diagonal_terrain_type, diagonal_mean

   if left_terrain_type then
      if top_terrain_type then
         diagonal_terrain_type = self:_get_terrain_type(zones, x-1, y-1)
         diagonal_mean = terrain_info[diagonal_terrain_type].mean_height 
         corner_mean = (left_mean + top_mean + diagonal_mean) * 0.333333333333333
         noise_map:set(1, 1, corner_mean)
         --offset = noise_map:get_offset(1, 1)
         --noise_map[offset] = noise_map[offset] - mean_height + corner_mean
      end
      if bottom_terrain_type then
         diagonal_terrain_type = self:_get_terrain_type(zones, x-1, y+1)
         diagonal_mean = terrain_info[diagonal_terrain_type].mean_height 
         corner_mean = (left_mean + bottom_mean + diagonal_mean) * 0.333333333333333
         noise_map:set(1, height, corner_mean)
         --offset = noise_map:get_offset(1, height)
         --noise_map[offset] = noise_map[offset] - mean_height + corner_mean
      end
   end

   if right_terrain_type then
      if top_terrain_type then
         diagonal_terrain_type = self:_get_terrain_type(zones, x+1, y-1)
         diagonal_mean = terrain_info[diagonal_terrain_type].mean_height 
         corner_mean = (right_mean + top_mean + diagonal_mean) * 0.333333333333333
         noise_map:set(width, 1, corner_mean)
         --offset = noise_map:get_offset(width, 1)
         --noise_map[offset] = noise_map[offset] - mean_height + corner_mean
      end
      if bottom_terrain_type then
         diagonal_terrain_type = self:_get_terrain_type(zones, x+1, y+1)
         diagonal_mean = terrain_info[diagonal_terrain_type].mean_height 
         corner_mean = (right_mean + bottom_mean + diagonal_mean) * 0.333333333333333
         noise_map:set(width, height, corner_mean)
         --offset = noise_map:get_offset(width, height)
         --noise_map[offset] = noise_map[offset] - mean_height + corner_mean
      end
   end

   -- these are corners with no defined neighbors
   -- avoid extreme values on corners
   -- this halves the standard deviation
   if not left_terrain_type and not top_terrain_type then
      offset = noise_map:get_offset(1, 1)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end

   if not left_terrain_type and not bottom_terrain_type then
      offset = noise_map:get_offset(1, height)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end

   if not right_terrain_type and not top_terrain_type then
      offset = noise_map:get_offset(width, 1)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end

   if not right_terrain_type and not bottom_terrain_type then
      offset = noise_map:get_offset(width, height)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end
end

function TerrainGenerator:_calc_edge_range(start, stop, start_map, stop_map)
   if start_map then start = start + 1 end
   if stop_map then stop = stop - 1 end
   return start, stop
end

-- Note: double blends the inner corners when blend_rows is on. Not a big deal.
function TerrainGenerator:_copy_zone_context(micro_map, zones, x, y, blend_rows)
   if zones == nil then return end

   local terrain_type = micro_map.terrain_type
   local noise_mean = self.terrain_info[terrain_type].mean_height
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   local i, adjacent_map, offset, adj_val, noise_val, blend_val

   adjacent_map = self:_get_existing_zone(zones, x-1, y)
   if adjacent_map then
      for i=1, micro_height do
         -- copy right edge of adjacent to left edge of current
         adj_val = adjacent_map:get(micro_width, i)
         micro_map:set(1, i, adj_val)

         if (blend_rows) then
            offset = micro_map:get_offset(2, i)
            noise_val = micro_map[offset]
            blend_val = self:_calc_blended_edge_value(adj_val, noise_val, noise_mean)
            micro_map[offset] = blend_val
         end
      end
   end

   adjacent_map = self:_get_existing_zone(zones, x+1, y)
   if adjacent_map then
      for i=1, micro_height do
         -- copy left edge of adjacent to right edge of current
         adj_val = adjacent_map:get(1, i)
         micro_map:set(micro_width, i, adj_val)

         if (blend_rows) then
            offset = micro_map:get_offset(micro_width-1, i)
            noise_val = micro_map[offset]
            blend_val = self:_calc_blended_edge_value(adj_val, noise_val, noise_mean)
            micro_map[offset] = blend_val
         end
      end
   end

   adjacent_map = self:_get_existing_zone(zones, x, y-1)
   if adjacent_map then
      for i=1, micro_width do
         -- copy bottom edge of adjacent to top edge of current
         adj_val = adjacent_map:get(i, micro_height)
         micro_map:set(i, 1, adj_val)

         if (blend_rows) then
            offset = micro_map:get_offset(i, 2)
            noise_val = micro_map[offset]
            blend_val = self:_calc_blended_edge_value(adj_val, noise_val, noise_mean)
            micro_map[offset] = blend_val
         end
      end
   end

   adjacent_map = self:_get_existing_zone(zones, x, y+1)
   if adjacent_map then
      for i=1, micro_width do
         -- copy top edge of adjacent to bottom edge of current
         adj_val = adjacent_map:get(i, 1)
         micro_map:set(i, micro_height, adj_val)

         if (blend_rows) then
            offset = micro_map:get_offset(i, micro_height-1)
            noise_val = micro_map[offset]
            blend_val = self:_calc_blended_edge_value(adj_val, noise_val, noise_mean)
            micro_map[offset] = blend_val
         end
      end
   end
end

function TerrainGenerator:_calc_blended_edge_value(adj_val, noise_val, noise_mean)
   -- extract the AC component and add to the DC mean
   -- equivalent to:
   -- return (noise_val-noise_mean) + (adj_val+noise_mean)/2
   --return noise_val + (adj_val-noise_mean)*0.5
   return noise_val - noise_mean + adj_val
   --return adj_val
end

function TerrainGenerator:_get_zone_proxy(zones, x, y)
   if x < 1 then return nil end
   if y < 1 then return nil end
   if x > zones.width then return nil end
   if y > zones.height then return nil end
   return zones:get(x, y)
end

function TerrainGenerator:_get_existing_zone(zones, x, y)
   local zone = self:_get_zone_proxy(zones, x, y)
   if zone == nil then return nil end
   if not zone.generated then return nil end
   return zone
end

function TerrainGenerator:_get_terrain_type(zones, x, y)
   local zone = self:_get_zone_proxy(zones, x, y)
   if zone == nil then return nil end
   return zone.terrain_type
end

function TerrainGenerator:_filter_and_quantize_noise_map(noise_map)
   local terrain_type = noise_map.terrain_type
   local width = noise_map.width
   local height = noise_map.height
   local filtered_map = HeightMap(width, height)

   filtered_map.terrain_type = terrain_type
   FilterFns.filter_2D_025(filtered_map, noise_map, width, height)

   -- non-uniform quantizer is not for the micro_map
   self:_quantize_height_map(filtered_map, false)
   filtered_map.generated = true

   return filtered_map
end

-- edge values may not change values! they are shared with the adjacent zone
function TerrainGenerator:_postprocess_micro_map(micro_map)
   local i, j

   for j=1, micro_map.height do
      for i=1, micro_map.width do
         -- no need for a separate destination map, can fill in place
         self:_fill_hole(micro_map, i, j)
      end
   end
end

function TerrainGenerator:_fill_hole(height_map, x, y)
   local width = height_map.width
   local height = height_map.height
   local offset = height_map:get_offset(x, y)
   local value = height_map[offset]
   local new_value = 2000000000
   local neighbor

   -- edge tiles should not be treated as holes
   if x == 1 or x == width then return end
   if y == 1 or y == height then return end

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

function TerrainGenerator:_create_macro_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local terrain_type = micro_map.terrain_type
   local micro_width = micro_map.width
   local micro_height = micro_map.height

   -- micro_map should already be quantized by this point
   -- otherwise quantize before wavelet shaping
   for j=1, micro_height do
      for i=1, micro_width do
         value = micro_map:get(i, j)
         oversize_map:set_block((i-1)*self.tile_size+1, (j-1)*self.tile_size+1,
            self.tile_size, self.tile_size, value)
      end
   end

   oversize_map.terrain_type = terrain_type
end

-- transform to frequncy domain and shape frequencies with exponential decay
function TerrainGenerator:_shape_height_map(height_map)
   local width = height_map.width
   local height = height_map.height
   local levels = self.wavelet_levels
   local freq_scaling_coeff = self.frequency_scaling_coeff

   Wavelet.DWT_2D(height_map, width, height, levels)
   WaveletFns.scale_high_freq(height_map, width, height, freq_scaling_coeff, levels)
   Wavelet.IDWT_2D(height_map, width, height, levels)
end

function TerrainGenerator:_extract_zone_map(oversize_map)
   local zone_map_origin = self.tile_size/2 + 1
   local zone_map = HeightMap(self.zone_size, self.zone_size)

   zone_map.terrain_type = oversize_map.terrain_type

   oversize_map:copy_block(zone_map, oversize_map,
      1, 1, zone_map_origin, zone_map_origin, self.zone_size, self.zone_size)

   return zone_map
end

function TerrainGenerator:_create_micro_map(oversize_map)
   local offset_map_origin = self.tile_size/2 + 1
   local offset_map_width = oversize_map.width - self.tile_size/2
   local offset_map_height = oversize_map.height - self.tile_size/2
   local offset_map = HeightMap(offset_map_width, offset_map_height)

   oversize_map:copy_block(offset_map, oversize_map,
      1, 1, offset_map_origin, offset_map_origin, offset_map_width, offset_map_height)

   local micro_width = FilterFns.calc_resampled_length(offset_map_width, self.tile_size)
   local micro_height = FilterFns.calc_resampled_length(offset_map_height, self.tile_size)
   local micro_map = HeightMap(micro_width, micro_height)

   FilterFns.downsample_2D(micro_map, offset_map,
      offset_map_width, offset_map_height, self.tile_size)

   return micro_map
end

function TerrainGenerator:_quantize_height_map(height_map, enable_nonuniform_quantizer)
   local terrain_info = self.terrain_info
   local terrain_type = height_map.terrain_type

   height_map:process_map(
      function (value)
         -- step_size depends on altitude and zone type
         local step_size = self:_get_step_size(terrain_type, value)
         if value <= step_size then return step_size end

         local quantized_value = MathFns.quantize(value, step_size)
         if not enable_nonuniform_quantizer then return quantized_value end

         -- disable fancy mode for Plains?
         --if value < 8 then return quantized_value end

         -- non-uniform quantizer
         local rounded_value, diff
         rounded_value = MathFns.round(value)
         diff = quantized_value - rounded_value

         if diff == step_size*0.5 then
            return rounded_value
         else
            return quantized_value
         end
      end
   )
end

function TerrainGenerator:_get_step_size(terrain_type, value)
   local terrain_info = self.terrain_info

   if value > terrain_info[TerrainType.Foothills].max_height then
      return terrain_info[TerrainType.Mountains].step_size
   end

   if value > terrain_info[TerrainType.Plains].max_height then
      return terrain_info[TerrainType.Foothills].step_size
   end

   if terrain_type == TerrainType.Plains then
      return terrain_info[TerrainType.Plains].step_size
   else
      return terrain_info[TerrainType.Foothills].step_size
   end
end

----------

function TerrainGenerator:_create_test_map(height_map)
   height_map:clear(8)
   height_map:set_block(192, 192, 32, 32, 16)
end

return TerrainGenerator
