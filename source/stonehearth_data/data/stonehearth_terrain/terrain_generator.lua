local TerrainGenerator = class()

local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local GaussianNoise = radiant.mods.require('/stonehearth_terrain/gaussian_noise.lua')
local InverseGaussianNoise = radiant.mods.require('/stonehearth_terrain/inverse_gaussian_noise.lua')
local filter_2D = radiant.mods.require('/stonehearth_terrain/filter/filter_2D.lua')
local filter_ls25 = radiant.mods.require('/stonehearth_terrain/filter/filter_ls25.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local WaveletFns = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet_fns.lua')
local Point_2D = radiant.mods.require('/stonehearth_terrain/point_2D.lua')

function TerrainGenerator:__init(tile_size)
   math.randomseed(3)
   self.tile_size = tile_size
   self.block_size = 32
   self.foothills_quantization_size = 8

   self._detail_noise_generator = InverseGaussianNoise(
      self.foothills_quantization_size-1, GaussianNoise.LOW_QUALITY
   )
end

-- this API will change to accomodate fitting with adjacent tiles and user templates
function TerrainGenerator:generate_tile()
   local height_map = HeightMap(self.tile_size, self.tile_size)
   local levels = 4
   local freq_scaling_coeff = 0.7

   self:_create_map_template(height_map)
   
   local width = height_map.width
   local height = height_map.height
   Wavelet.DWT_2D(height_map, width, height, 1, levels)
   WaveletFns.scale_high_freq(height_map, width, height, 1, levels, freq_scaling_coeff)
   Wavelet.IDWT_2D(height_map, width, height, levels)

   height_map:quantize_map_height(self.foothills_quantization_size)
   self:_add_detail_blocks(height_map)

   -- why doesn't C++ like height of 0? Throws exception in Cube.cpp
   -- need to figure out baseline height later
   height_map:normalize_map_height(self.foothills_quantization_size)

   return height_map
end

function TerrainGenerator:_create_map_template(height_map)
   local i, j, value, vertical_scaling_factor
   local mini_width = height_map.width / self.block_size
   local mini_height = height_map.height / self.block_size
   local temp_map = HeightMap(mini_width, mini_height)

   self:_fill_map_with_gaussian_noise(temp_map, 100, GaussianNoise.LOW_QUALITY)

   local lowpass_filter = filter_ls25
   filter_2D(temp_map, mini_width, mini_height, lowpass_filter)

   vertical_scaling_factor = 2.5

   for j=1, mini_height, 1 do
      for i=1, mini_width, 1 do
         value = temp_map:get(i, j) * vertical_scaling_factor
         height_map:set_block((i-1)*self.block_size+1, (j-1)*self.block_size+1,
            self.block_size, self.block_size, value)
      end
   end

   height_map:normalize_map_height(self.foothills_quantization_size)
   height_map:quantize_map_height(self.foothills_quantization_size)
end

-- bad boundary conditions on small maps
function TerrainGenerator:_create_map_template_old(height_map)
   local i, j, value, vertical_scaling_factor
   local mini_width = height_map.width / self.block_size
   local mini_height = height_map.height / self.block_size
   local temp_map = HeightMap(mini_width, mini_height)

   self:_fill_map_with_gaussian_noise(temp_map, 100, GaussianNoise.LOW_QUALITY)

   Wavelet.DWT_2D(temp_map, mini_width, mini_height, 1, 2)
   WaveletFns.scale_high_freq(temp_map, mini_width, mini_height, 1, 2, 0)
   Wavelet.IDWT_2D(temp_map, mini_width, mini_height, 2, 1)

   vertical_scaling_factor = 4

   for j=1, mini_height, 1 do
      for i=1, mini_width, 1 do
         value = temp_map:get(i, j) * vertical_scaling_factor
         height_map:set_block((i-1)*self.block_size+1, (j-1)*self.block_size+1,
            self.block_size, self.block_size, value)
      end
   end

   height_map:normalize_map_height(self.foothills_quantization_size)
   height_map:quantize_map_height(self.foothills_quantization_size)
end

function TerrainGenerator:_fill_map_with_gaussian_noise(height_map, amplitude, quality)
   local gaussian_noise = GaussianNoise(amplitude, GaussianNoise.LOW_QUALITY)

   height_map:process_map(
      function ()
         return gaussian_noise:generate()
      end
   )
end

function TerrainGenerator:_add_detail_blocks(height_map)
   local i, j
   local is_lower_edge
   local edge_map = HeightMap(height_map.width, height_map.height)
   local roll
   local seed_probability = 0.10
   local detail_seeds = {}
   local num_seeds = 0

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         is_lower_edge = height_map:has_higher_neighbor(i, j)
         edge_map:set(i, j, is_lower_edge)

         if is_lower_edge then
            if math.random() < seed_probability then
               num_seeds = num_seeds + 1
               detail_seeds[num_seeds] = Point_2D(i, j)
            end
         end
      end
   end

   local x, y, point
   for i=1, num_seeds, 1 do
      point = detail_seeds[i]
      self:_grow_seed(height_map, edge_map, point.x, point.y)
   end
end

function TerrainGenerator:_grow_seed(height_map, edge_map, x, y)
   if not edge_map:get(x, y) then return false end

   local i, j, continue, value, detail_height

   detail_height = self:_generate_detail_height()

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

function TerrainGenerator:_generate_detail_height()
   -- generate from 1 to quantization_size
   return self._detail_noise_generator:generate() + 1
end

function TerrainGenerator:_try_grow(height_map, edge_map, x, y, detail_height)
   local value
   local grow_probability = 0.75

   if not edge_map:get(x, y) then return false end
   if math.random() >= grow_probability then return false end
   value = height_map:get(x, y)
   height_map:set(x, y, value+detail_height)
   edge_map:set(x, y, false)
   return true
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

   height_map:normalize_map_height(self.foothills_quantization_size)
   height_map:quantize_map_height(self.foothills_quantization_size)

   return height_map
end

return TerrainGenerator

