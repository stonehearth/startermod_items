local TerrainGenerator = class()

local ZoneType = radiant.mods.require('/stonehearth_terrain/zone_type.lua')
local Array2D = radiant.mods.require('/stonehearth_terrain/array_2D.lua')
local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
local InverseGaussianRandom = radiant.mods.require('/stonehearth_terrain/math/inverse_gaussian_random.lua')
local MathFns = radiant.mods.require('/stonehearth_terrain/math/math_fns.lua')
local FilterFns = radiant.mods.require('/stonehearth_terrain/filter/filter_fns.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local WaveletFns = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet_fns.lua')
local Point_2D = radiant.mods.require('/stonehearth_terrain/point_2D.lua')

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (foothills, plains, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init()
   local zone_type
   math.randomseed(3)

   self.zone_size = 256
   self.tile_size = 32

   self.wavelet_levels = 4
   self.frequency_scaling_coeff = 0.7

   self.detail_seed_probability = 0.10
   self.detail_grow_probability = 0.85

   self.zone_params = {}

   zone_type = ZoneType.Plains
   self.zone_params[zone_type] = {}
   self.zone_params[zone_type].quantization_size = 2
   self.zone_params[zone_type].mean_height = 8
   self.zone_params[zone_type].std_dev = 4

   zone_type = ZoneType.Foothills
   self.zone_params[zone_type] = {}
   self.zone_params[zone_type].quantization_size = 8
   self.zone_params[zone_type].mean_height = 24
   self.zone_params[zone_type].std_dev = 32

   zone_type = ZoneType.Mountains
   self.zone_params[zone_type] = {}
   self.zone_params[zone_type].quantization_size = 8
   self.zone_params[zone_type].mean_height = 48
   self.zone_params[zone_type].std_dev = 48

   local oversize_zone_size = self.zone_size + self.tile_size
   self.oversize_map_buffer = HeightMap(oversize_zone_size, oversize_zone_size)
end

function TerrainGenerator:generate_zone(zone_type, zones, x, y)
   local zone_map, micro_map, noise_map
   local quantization_size = self.zone_params[zone_type].quantization_size
   local oversize_map = self.oversize_map_buffer
   local micro_width = oversize_map.width / self.tile_size
   local micro_height = oversize_map.height / self.tile_size
   oversize_map.zone_type = zone_type

   noise_map = self:_generate_noise_map(zone_type, micro_width, micro_height)

   -- propagate edge information from surrounding zones into current map
   self:_copy_zone_context(noise_map, zones, x, y, true)

   micro_map = self:_filter_and_quantize_noise_map(noise_map)

   -- force shared tiles to same value from adjacent zones
   self:_copy_zone_context(micro_map, zones, x, y, false)

   self:_create_macro_map_from_micro_map(oversize_map, micro_map)
   
   self:_shape_height_map(oversize_map)

   self:_quantize_height_map_nonuniform(oversize_map, quantization_size)
   --self:_quantize_height_map(oversize_map, quantization_size)

   self:_add_detail_blocks(oversize_map, zone_type)

   -- copy the offset zone map from the oversize map
   zone_map = self:_create_zone_map(oversize_map)

   return zone_map, micro_map
end

function TerrainGenerator:_generate_noise_map(zone_type, width, height)
   local noise_map = HeightMap(width, height)
   local std_dev = self.zone_params[zone_type].std_dev
   local mean_height = self.zone_params[zone_type].mean_height

   noise_map.zone_type = zone_type

   noise_map:process_map(
      function ()
         return GaussianRandom.generate(mean_height, std_dev)
      end
   )

   -- avoid extreme values on corners
   local offset
   offset = noise_map:get_offset(1, 1)
   noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

   offset = noise_map:get_offset(width, 1)
   noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

   offset = noise_map:get_offset(1, height)
   noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

   offset = noise_map:get_offset(width, height)
   noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

   --[[
   -- avoid extreme values along edges
   local i
   for i=1, width, 1 do
      offset = noise_map:get_offset(i, 1)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

      offset = noise_map:get_offset(i, height)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end

   for i=2, height-1, 1 do
      offset = noise_map:get_offset(1, i)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5

      offset = noise_map:get_offset(width, i)
      noise_map[offset] = (noise_map[offset] + mean_height) * 0.5
   end
   ]]
   return noise_map
end

function TerrainGenerator:_copy_zone_context(micro_map, zones, x, y, blend_rows)
   if zones == nil then return end

   local zone_type = micro_map.zone_type
   local noise_mean = self.zone_params[zone_type].mean_height
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   local i, adjacent_map, offset, adj_val, noise_val, blend_val

   adjacent_map = self:_get_zone(zones, x-1, y)
   if adjacent_map then
      for i=1, micro_height, 1 do
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

   adjacent_map = self:_get_zone(zones, x+1, y)
   if adjacent_map then
      for i=1, micro_height, 1 do
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

   adjacent_map = self:_get_zone(zones, x, y-1)
   if adjacent_map then
      for i=1, micro_width, 1 do
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

   adjacent_map = self:_get_zone(zones, x, y+1)
   if adjacent_map then
      for i=1, micro_width, 1 do
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
   --return noise_val - noise_mean + adj_val
   return adj_val
end

function TerrainGenerator:_get_zone(zones, x, y)
   if x < 1 then return nil end
   if y < 1 then return nil end
   if x > zones.width then return nil end
   if y > zones.height then return nil end
   return zones:get(x, y)
end

function TerrainGenerator:_filter_and_quantize_noise_map(noise_map)
   local zone_type = noise_map.zone_type
   local width = noise_map.width
   local height = noise_map.height
   local filtered_map = HeightMap(width, height)

   filtered_map.zone_type = zone_type
   FilterFns.filter_2D_025(filtered_map, noise_map, width, height)

   local quantization_size
   quantization_size = self.zone_params[zone_type].quantization_size
   self:_quantize_height_map(filtered_map, quantization_size)

   return filtered_map
end

function TerrainGenerator:_create_macro_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local zone_type = micro_map.zone_type
   local micro_width = micro_map.width
   local micro_height = micro_map.height

   -- micro_map should already be quantized by this point
   -- otherwise quantize before wavelet shaping
   for j=1, micro_height, 1 do
      for i=1, micro_width, 1 do
         value = micro_map:get(i, j)
         oversize_map:set_block((i-1)*self.tile_size+1, (j-1)*self.tile_size+1,
            self.tile_size, self.tile_size, value)
      end
   end

   oversize_map.zone_type = zone_type
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

function TerrainGenerator:_create_zone_map(oversize_map)
   local zone_map_origin = self.tile_size/2 + 1
   local zone_map = HeightMap(self.zone_size, self.zone_size)

   zone_map.zone_type = oversize_map.zone_type

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

function TerrainGenerator:_quantize_height_map(height_map, step_size)
   height_map:process_map(
      function (value)
         return MathFns.quantize(value, step_size)
      end
   )
end

function TerrainGenerator:_quantize_height_map_nonuniform(height_map, step_size)
   local half_step = step_size * 0.5
   height_map:process_map(
      function (value)
         local rounded_value, quantized_value, diff
         quantized_value = MathFns.quantize(value, step_size)
         rounded_value = MathFns.round(value)
         diff = quantized_value - rounded_value
         --if diff >= 3 and diff <= half_step then
         if diff == half_step then
            return rounded_value
         else
            return quantized_value
         end
      end
   )
end

function TerrainGenerator:_add_detail_blocks(height_map, zone_type)
   local i, j
   local edge
   local edge_map = HeightMap(height_map.width, height_map.height)
   local roll
   local detail_seeds = {}
   local num_seeds = 0
   local quantization_size = self.zone_params[zone_type].quantization_size
   local edge_threshold = quantization_size/2

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         edge = self:_is_edge(height_map, i, j, edge_threshold)
         edge_map:set(i, j, edge)

         if edge then
            if math.random() < self.detail_seed_probability then
               num_seeds = num_seeds + 1
               detail_seeds[num_seeds] = Point_2D(i, j)
            end
         end
      end
   end

   local point
   for i=1, num_seeds, 1 do
      point = detail_seeds[i]
      self:_grow_seed(height_map, edge_map, point.x, point.y)
   end
end

function TerrainGenerator:_grow_seed(height_map, edge_map, x, y)
   local edge = edge_map:get(x, y)
   if edge == false then return end

   local i, j, continue, value, detail_height

   -- if edge is not false, edge is the dela to the highest neighbor
   detail_height = self:_generate_detail_height(edge)

   value = height_map:get(x, y)
   height_map:set(x, y, value+detail_height)
   edge_map:set(x, y, false)

   i = x
   j = y
   while true do
      -- grow left
      i = i - 1
      if i < 1 then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
      -- grow right
      i = i + 1
      if i > height_map.width then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
      -- grow up
      j = j - 1
      if j < 1 then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
      -- grow down
      j = j + 1
      if j > height_map.height then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end
end

-- inverse bell curve from 1 to quantization size
-- extend range by 0.5 on each end so that endpoint distribution is not clipped
function TerrainGenerator:_generate_detail_height(max_delta)
   local min = 0.5
   local max = max_delta + 0.49

   -- place the midpoint 2 standard deviations away
   local std_dev = (max-min)*0.25
   local rand = InverseGaussianRandom.generate(min, max, std_dev)
   return MathFns.round(rand)
end

function TerrainGenerator:_generate_detail_height_uniform(max_delta)
   return math.random(1, max_delta)
end

function TerrainGenerator:_try_grow(height_map, edge_map, x, y, detail_height)
   local edge, value

   edge = edge_map:get(x, y)
   if edge == false then return false end
   if math.random() >= self.detail_grow_probability then return false end

   detail_height = math.min(detail_height, edge)

   value = height_map:get(x, y)
   height_map:set(x, y, value+detail_height)
   edge_map:set(x, y, false)
   return true
end

-- returns false is no non-diagonal neighbor higher by more then threshold
-- othrewise returns the height delta to the highest neighbor
function TerrainGenerator:_is_edge(height_map, x, y, threshold)
   if threshold == nil then threshold = 0.99 end
   local neighbor
   local offset = height_map:get_offset(x, y)
   local value = height_map[offset]
   local delta
   local max_delta = threshold

   if x > 1 then
      neighbor = height_map[offset-1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if x < height_map.width then
      neighbor = height_map[offset+1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y > 1 then
      neighbor = height_map[offset-height_map.width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y < height_map.height then
      neighbor = height_map[offset+height_map.width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end

   if max_delta > threshold then
      return max_delta
   else
      return false
   end
end

----------

function TerrainGenerator:_create_test_map(height_map)
   height_map:clear(8)
   height_map:set_block(192, 192, 32, 32, 16)
end

function TerrainGenerator:_erosion_test()
   local levels = 4
   local coeff = 0.5
   local height_map = HeightMap(128, 128)
   local quantization_size = self.zone_params[ZoneType.Foothills].quantization_size

   height_map:process_map(
      function () return self.quantization_size end
   )

   height_map:set_block(32, 32, 32, 32, self.quantization_size*2)

   local width = height_map.width
   local height = height_map.height
   Wavelet.DWT_2D(height_map, width, height, 1, levels)
   WaveletFns.scale_high_freq(height_map, width, height, 1, levels, coeff)
   Wavelet.IDWT_2D(height_map, width, height, levels)

   self:_quantize_height_map(height_map, self.quantization_size)

   return height_map
end

return TerrainGenerator
