local TerrainGenerator = class()

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

function TerrainGenerator:__init(zone_size)
   math.randomseed(3)
   self.zone_size = zone_size
   self.tile_size = 32
   self.foothills_quantization_size = 8
   self.foothills_mean_height = 16
   self.foothills_std_dev = 16
end

function TerrainGenerator:generate_zone(zones, x, y)
   local oversize_zone_size = self.zone_size + self.tile_size
   local oversize_map = HeightMap(oversize_zone_size, oversize_zone_size)
   local sub_map = HeightMap(self.zone_size, self.zone_size)
   local sub_map_origin = self.tile_size/2 + 1

   self:_create_zone_template(oversize_map)
   
   self:_shape_height_map(oversize_map)

   --self:_quantize_map_height_nonuniform(oversize_map, self.foothills_quantization_size)
   self:_quantize_map_height(oversize_map, self.foothills_quantization_size)
   self:_add_detail_blocks(oversize_map)

   oversize_map:copy_block(sub_map, oversize_map,
      1, 1, sub_map_origin, sub_map_origin, self.zone_size, self.zone_size)

   return sub_map
end

function TerrainGenerator:_create_micro_map(height_map)
   local micro_width = height_map.width / self.tile_size
   local micro_height = height_map.height / self.tile_size
   local micro_map = HeightMap(micro_width, micro_height)

   FilterFns.downsample_2D(micro_map, height_map,
      height_map.width, height_map.height, self.tile_size)

   return micro_map
end

function TerrainGenerator:_create_zone_template(height_map)
   local i, j, value, vertical_scaling_factor
   local mini_width = height_map.width / self.tile_size
   local mini_height = height_map.height / self.tile_size
   local noise_map
   local filtered_map = HeightMap(mini_width, mini_height)

   noise_map = self:_generate_random_template(mini_width, mini_height)

   FilterFns.filter_2D_025(filtered_map, noise_map, mini_width, mini_height)

   vertical_scaling_factor = 2.2

   for j=1, mini_height, 1 do
      for i=1, mini_width, 1 do
         value = filtered_map:get(i, j) * vertical_scaling_factor
         height_map:set_block((i-1)*self.tile_size+1, (j-1)*self.tile_size+1,
            self.tile_size, self.tile_size, value)
      end
   end

   self:_quantize_map_height(height_map, self.foothills_quantization_size)
end

-- transform to frequncy domain and shape frequencies with exponential decay
function TerrainGenerator:_shape_height_map(height_map)
   local width = height_map.width
   local height = height_map.height
   local levels = 4
   local freq_scaling_coeff = 0.7

   Wavelet.DWT_2D(height_map, width, height, levels)
   WaveletFns.scale_high_freq(height_map, width, height, freq_scaling_coeff, levels)
   Wavelet.IDWT_2D(height_map, width, height, levels)
end

function TerrainGenerator:_generate_random_template(width, height)
   local template = HeightMap(width, height)

   template:process_map(
      function ()
         return GaussianRandom.generate(self.foothills_mean_height, self.foothills_std_dev)
      end
   )

   return template
end

function TerrainGenerator:_zone_exists(zones, x, y)
   if x < 1 then return false end
   if y < 1 then return false end
   if x > zones.width then return false end
   if y > zones.height then return false end
   return zones:get(x, y)
end

function TerrainGenerator:_quantize_map_height(height_map, step_size)
   height_map:process_map(
      function (value)
         return MathFns.quantize(value, step_size)
      end
   )
end

function TerrainGenerator:_quantize_map_height_nonuniform(height_map, step_size)
   local half_step = step_size * 0.5
   local rounded_value, quantized_value, diff
   height_map:process_map(
      function (value)
         quantized_value = MathFns.quantize(value, step_size)
         rounded_value = MathFns.round(value)
         diff = quantized_value - rounded_value
         if diff >= 3 and diff <= half_step then
            return rounded_value
         else
            return quantized_value
         end
      end
   )
end

function TerrainGenerator:_add_detail_blocks(height_map)
   local i, j
   local edge
   local edge_map = HeightMap(height_map.width, height_map.height)
   local roll
   local seed_probability = 0.10
   local detail_seeds = {}
   local num_seeds = 0
   local edge_threshold = self.foothills_quantization_size/2

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         edge = self:_is_edge(height_map, i, j, edge_threshold)
         edge_map:set(i, j, edge)

         if edge then
            if math.random() < seed_probability then
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
      i = i - 1
      if i < 1 then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
      i = i + 1
      if i > height_map.width then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
      j = j - 1
      if j < 1 then break end
      continue = self:_try_grow(height_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i = x
   j = y
   while true do
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
   local max = math.min(max_delta, self.foothills_quantization_size) + 0.49

   -- place the midpoint 2 standard deviations away
   local std_dev = (max-min)*0.25
   local rand = InverseGaussianRandom.generate(min, max, std_dev)
   return MathFns.round(rand)
end

function TerrainGenerator:_generate_detail_height_uniform(max_delta)
   local min = 1
   local max = math.min(max_delta, self.foothills_quantization_size)
   return math.random(min, max)
end

function TerrainGenerator:_try_grow(height_map, edge_map, x, y, detail_height)
   local edge, value
   local grow_probability = 0.85

   edge = edge_map:get(x, y)
   if edge == false then return false end
   if math.random() >= grow_probability then return false end

   detail_height = math.min(detail_height, edge) -- CHECKCHECK

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
   height_map:process_map(function (value) return 16 end)
   height_map:process_map_block(32, 32, 32, 32, function (value) return 14 end)
end

function TerrainGenerator:_erosion_test()
   local levels = 4
   local coeff = 0.5
   local height_map = HeightMap(128, 128)

   height_map:process_map(
      function () return self.foothills_quantization_size end
   )

   height_map:set_block(32, 32, 32, 32, self.foothills_quantization_size*2)

   local width = height_map.width
   local height = height_map.height
   Wavelet.DWT_2D(height_map, width, height, 1, levels)
   WaveletFns.scale_high_freq(height_map, width, height, 1, levels, coeff)
   Wavelet.IDWT_2D(height_map, width, height, levels)

   self:_quantize_map_height(height_map, self.foothills_quantization_size)

   return height_map
end

return TerrainGenerator

