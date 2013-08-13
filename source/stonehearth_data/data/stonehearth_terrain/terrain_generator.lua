local TerrainGenerator = class()

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local GaussianNoise = radiant.mods.require('/stonehearth_terrain/gaussian_noise.lua')
local InverseGaussianNoise = radiant.mods.require('/stonehearth_terrain/inverse_gaussian_noise.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local WaveletFns = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet_fns.lua')
local Point2D = radiant.mods.require('/stonehearth_terrain/point_2D.lua')

function TerrainGenerator:__init(tile_size)
   math.randomseed(2)
   self.tile_size = tile_size
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
   Wavelet:DWT_2D(height_map, width, height, 1, levels)
   WaveletFns:scale_high_freq(height_map, width, height, 1, levels, freq_scaling_coeff)
   Wavelet:IDWT_2D(height_map, width, height, levels)

   height_map:quantize_map_height(self.foothills_quantization_size)
   self:_add_detail_blocks(height_map)

   -- why doesn't C++ like height of 0? Throws exception in Cube.cpp
   -- need to figure out baseline height later
   height_map:normalize_map_height(self.foothills_quantization_size)

   self:_render_height_map_to_terrain(height_map)
end

function TerrainGenerator:_create_map_template(height_map)
   local temp_map = HeightMap(self.tile_size, self.tile_size)
   local levels = 5
   local filter_levels = 1 -- levels to filter the LL sample map
   --local freq_scaling_coeff = 0.125
   local freq_scaling_coeff = 0.0
   local width = temp_map.width
   local height = temp_map.height

   -- generate sample terrain into temp_map
   self:_fill_map_with_gaussian_noise(temp_map, 100, 3)

   Wavelet:DWT_2D(temp_map, width, height, 1, levels+filter_levels)
   WaveletFns:scale_high_freq(temp_map, width, height, 1, levels+filter_levels, freq_scaling_coeff)
   Wavelet:IDWT_2D(temp_map, width, height, levels+filter_levels, levels+1)

   -- normalize LL component back to same scale as original
   -- conservation of energy in transform
   -- 1/sqrt(2) per 1D transform, so 1/2 per 2D transform
   -- will lose energy to high frequency bands
   local energy_scaling = (0.5)^levels
   local map_type_scaling = self.foothills_quantization_size*4
   local scaling_factor = energy_scaling * map_type_scaling

   -- blockify sample terrain for use as the map template
   local i, j, value
   local block_width, block_height
   local level_width, level_height
   -- add 1 since we want the dimensions of the LL (low frequency) block of level 4
   level_width, level_height = Wavelet:get_dimensions_for_level(width, height, levels+1)
   block_width = width / level_width
   block_height = height / level_height

   radiant.log.info("Terrain template blocksize: %d, %d", block_width, block_height)

   for j=1, level_height, 1 do
      for i=1, level_width, 1 do
         value = temp_map:get(i, j) * scaling_factor
         height_map:set_block((i-1)*block_width+1, (j-1)*block_height+1,
            block_width, block_height, value)
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
               detail_seeds[num_seeds] = Point2D(i, j)
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

----------

function TerrainGenerator:_render_height_map_to_terrain(height_map)
   local r2 = Region2()
   local r3 = Region3()
   local terrain = radiant._root_entity:add_component('terrain')
   local heightMapCPP = HeightMapCPP(self.tile_size, 1)

   self:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   _radiant.csg.convert_heightmap_to_region2(heightMapCPP, r2)

   for rect in r2:contents() do
   --if rect.tag > 0 then
      self:_add_land_to_region(r3, rect, rect.tag);         
   --end
   end

   terrain:add_region(r3)
end

function TerrainGenerator:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   local row_offset = 0

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         heightMapCPP:set(i-1, j-1, math.abs(height_map[row_offset+i]))
      end
      row_offset = row_offset + height_map.width
   end
end

function TerrainGenerator:_add_land_to_region(dst, rect, height)
   dst:add_cube(Cube3(Point3(rect.min.x, -2,       rect.min.y),
                      Point3(rect.max.x, 0,        rect.max.y),
                Terrain.BEDROCK))

   if height % self.foothills_quantization_size == 0 then
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.GRASS))
   else
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.TOPSOIL))
   end
end

----------

function TerrainGenerator:_erosion_test()
   local levels = 4
   local coeff = 0.9
   local height_map = HeightMap(tile_size, tile_size)
   height_map:set_block(1, 1, tile_size, tile_size, 1)
   height_map:set_block(1, 1, 16, 16, 11)

   local width = height_map.width
   local height = height_map.height
   Wavelet:DWT_2D(height_map, width, height, 1, levels)
   WaveletFns:scale_high_freq(height_map, width, height, 1, levels, coeff)
   Wavelet:IDWT_2D(height_map, width, height, levels)

   height_map:normalize_map_height(quantization_size)
   height_map:quantize_map_height(quantization_size)

   self:_render_height_map_to_terrain(height_map)
end

return TerrainGenerator

