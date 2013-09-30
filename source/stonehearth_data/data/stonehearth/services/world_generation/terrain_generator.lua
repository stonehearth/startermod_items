local TerrainType = require 'services.world_generation.terrain_type'
local Array2D = require 'services.world_generation.array_2D'
local HeightMap = require 'services.world_generation.height_map'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
local MathFns = require 'services.world_generation.math.math_fns'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local Wavelet = require 'services.world_generation.wavelet.wavelet'
local WaveletFns = require 'services.world_generation.wavelet.wavelet_fns'
local EdgeDetailer = require 'services.world_generation.edge_detailer'
local TileInfo = require 'services.world_generation.tile_info'
local Timer = radiant.mods.require('stonehearth_debugtools.timer')

local TerrainGenerator = class()

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (foothills, plains, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init()
   local terrain_type, terrain_info

   self.base_random_seed = 5

   self.zone_size = 256
   self.tile_size = 32

   self.wavelet_levels = 4
   self.frequency_scaling_coeff = 0.7

   local base_step_size = 8
   self.terrain_info = {}
   terrain_info = self.terrain_info

   local plains_info = {}
   plains_info.step_size = 2
   plains_info.mean_height = 16
   plains_info.std_dev = 8
   plains_info.min_height = 12
   plains_info.max_height = 16
   terrain_info[TerrainType.Plains] = plains_info
   assert(plains_info.max_height % plains_info.step_size == 0)

   local foothills_info = {}
   foothills_info.step_size = base_step_size
   foothills_info.mean_height = 24
   foothills_info.std_dev = 32
   foothills_info.min_height = plains_info.max_height
   foothills_info.max_height = 32
   terrain_info[TerrainType.Foothills] = foothills_info
   assert(foothills_info.max_height % foothills_info.step_size == 0)

   local mountains_info = {}
   mountains_info.step_size = base_step_size*4
   mountains_info.mean_height = 96
   mountains_info.std_dev = 128
   mountains_info.min_height = plains_info.max_height + foothills_info.step_size
   terrain_info[TerrainType.Mountains] = mountains_info

   -- make sure that next step size is a multiple of the prior max height
   assert(plains_info.max_height % foothills_info.step_size == 0)
   assert(foothills_info.max_height % mountains_info.step_size == 0)

   terrain_info.tree_line = foothills_info.max_height + mountains_info.step_size*2
   terrain_info.max_deciduous_height = foothills_info.max_height
   terrain_info.min_evergreen_height = plains_info.max_height + 1

   local oversize_zone_size = self.zone_size + self.tile_size
   self.oversize_map_buffer = HeightMap(oversize_zone_size, oversize_zone_size)

   local micro_size = oversize_zone_size / self.tile_size
   self.blend_map_buffer = self:_create_blend_map(micro_size, micro_size)
   self.noise_map_buffer = HeightMap(micro_size, micro_size)

   self._edge_detailer = EdgeDetailer(terrain_info)
end

function TerrainGenerator:set_async(async)
   self._is_async = async
end

function TerrainGenerator:_set_random_seed(x, y)
   local seed = self.base_random_seed
   if x ~= nil and y ~= nil then 
      seed = seed + y*10000 + x*100
   end
   math.randomseed(seed)
end

function TerrainGenerator:_create_blend_map(width, height)
   local i, j
   local blend_map = Array2D(width, height)

   for j=1, width do
      for i=1, height do
         blend_map:set(i, j, TileInfo())
      end
   end

   return blend_map
end

function TerrainGenerator:generate_zone(terrain_type, zones, x, y)
   local oversize_map = self.oversize_map_buffer
   local blend_map = self.blend_map_buffer
   local noise_map = self.noise_map_buffer
   local micro_map, zone_map
   local timer = Timer(Timer.CPU_TIME)

   if zones ~= nil then
      radiant.log.info('Generating zone %d, %d', x, y)
   end
   self:_set_random_seed(x, y)
   timer:start()

   blend_map.terrain_type = terrain_type
   noise_map.terrain_type = terrain_type

   self:_fill_blend_map(blend_map, zones, x, y)

   self:_fill_noise_map(noise_map, blend_map)

   -- micro_map must be a new HeightMap as it is returned from the function
   micro_map = self:_filter_noise_map(noise_map)

   self:_quantize_micro_map(micro_map)

   -- force shared tiles to same value from adjacent zones
   self:_copy_forced_edge_values(micro_map, zones, x, y)

   self:_postprocess_micro_map(micro_map)

   -- radiant.log.info('Blend map:'); _print_blend_map(blend_map)
   -- radiant.log.info('Noise map:'); noise_map:print()
   -- radiant.log.info('Filtered Noise map:'); micro_map:print()
   -- radiant.log.info('Quantized Micro map:'); micro_map:print()
   -- radiant.log.info('Postprocessed Micro map:'); micro_map:print()

   -- zone_map must be a new HeightMap as it is returned from the function
   self:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   self:_yield()

   -- transform to frequncy domain and shape frequencies with exponential decay
   self:_shape_height_map(oversize_map, self.frequency_scaling_coeff, self.wavelet_levels)

   -- enable non-uniform quantizer within a terrain type
   -- generates additional edge details
   self:_quantize_height_map(oversize_map, true)
   self:_yield()

   self:_add_additional_details(oversize_map)

   -- copy the offset zone map from the oversize map
   zone_map = self:_extract_zone_map(oversize_map)
   self:_yield()

   timer:stop()
   radiant.log.info('Zone generation time: %.3fs', timer:seconds())

   return zone_map, micro_map
end

function TerrainGenerator:_yield()
   if self._is_async then
      -- doesn't really matter what we yield.  just giving up control
      coroutine.yield('...')
   end
end
function TerrainGenerator:_fill_blend_map(blend_map, zones, x, y)
   local i, j, adjacent_zone, tile
   local width = blend_map.width
   local height = blend_map.height
   local terrain_type = blend_map.terrain_type
   local terrain_mean = self.terrain_info[terrain_type].mean_height
   local terrain_std_dev = self.terrain_info[terrain_type].std_dev

   for j=1, height do
      for i=1, width do
         tile = blend_map:get(i, j)
         tile:clear()
         tile.mean = terrain_mean
         tile.std_dev = terrain_std_dev
         tile.mean_weight = 1
         tile.std_dev_weight = 1
      end
   end

   if zones ~= nil then
      -- blend values with left zone
      adjacent_zone = self:_get_zone(zones, x-1, y)
      self:_blend_zone(blend_map, adjacent_zone, height,      1,  1,  width, 1)

      -- blend values with right zone
      adjacent_zone = self:_get_zone(zones, x+1, y)
      self:_blend_zone(blend_map, adjacent_zone, height,  width, -1,      1, 1)

      -- blend values with top zone
      adjacent_zone = self:_get_zone(zones, x, y-1)
      self:_blend_zone(blend_map, adjacent_zone,  width,      1,  1, height, 2)

      -- blend values with bottom zone
      adjacent_zone = self:_get_zone(zones, x, y+1)
      self:_blend_zone(blend_map, adjacent_zone,  width, height, -1,      1, 2)
   end

   -- avoid extreme values on corners by halving std_dev
   _halve_tile_std_dev(blend_map:get(1, 1))
   _halve_tile_std_dev(blend_map:get(width, 1))
   _halve_tile_std_dev(blend_map:get(1,     height))
   _halve_tile_std_dev(blend_map:get(width, height))

   return blend_map
end

function _halve_tile_std_dev(tile)
   if tile.std_dev then tile.std_dev = tile.std_dev * 0.5 end
end

function _get_coords(x, y, orientation)
   if orientation == 1 then return x, y
   else                     return y, x end
end

function TerrainGenerator:_blend_zone(blend_map, adj_zone,
   edge_length, edge_index, inc, adj_edge_index, orientation)
   if adj_zone == nil then return end

   local adj_terrain_type = adj_zone.terrain_type
   local adj_mean = self.terrain_info[adj_terrain_type].mean_height
   local adj_std_dev = self.terrain_info[adj_terrain_type].std_dev
   local blend_tile
   local cur_tile = TileInfo()
   local i, edge_value, x, y

   for i=1, edge_length do
      -- row/column on edge
      cur_tile:clear()
      if adj_zone.generated then
         -- force edge values since they are shared
         x, y = _get_coords(adj_edge_index, i, orientation)
         edge_value = adj_zone:get(x, y)
         cur_tile.forced_value = edge_value
      else
         cur_tile.mean = adj_mean
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 1
         cur_tile.std_dev_weight = 1
      end
      x, y = _get_coords(edge_index, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)

      -- 2nd row/column
      -- refactor this after we settle on weights
      cur_tile:clear()
      if adj_zone.generated then
         cur_tile.mean = edge_value
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 3
         cur_tile.std_dev_weight = 1
      else
         cur_tile.mean = adj_mean
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 1
         cur_tile.std_dev_weight = 1
      end
      x, y = _get_coords(edge_index+inc, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)

      -- 3rd row/column
      -- refactor this after we settle on weights
      cur_tile:clear()
      if adj_zone.generated then
         cur_tile.mean = edge_value
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 1
         cur_tile.std_dev_weight = 1
      else
         cur_tile.mean = adj_mean
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 0.33
         cur_tile.std_dev_weight = 0.33
      end
      x, y = _get_coords(edge_index+inc+inc, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)

      -- 4th row/column
      -- refactor this after we settle on weights
      cur_tile:clear()
      if adj_zone.generated then
         cur_tile.mean = edge_value
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 0.33
         cur_tile.std_dev_weight = 0.33
      else
         cur_tile.mean = adj_mean
         cur_tile.std_dev = adj_std_dev
         cur_tile.mean_weight = 0.11
         cur_tile.std_dev_weight = 0.11
      end
      x, y = _get_coords(edge_index+inc+inc, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)
   end
end

function TerrainGenerator:_fill_noise_map(noise_map, blend_map)
   local width = noise_map.width
   local height = noise_map.height
   local i, j, tile, value

   for j=1, height do
      for i=1, width do
         tile = blend_map:get(i, j)
         if tile.forced_value then
            value = tile.forced_value
         else
            value = GaussianRandom.generate(tile.mean, tile.std_dev)
         end
         noise_map:set(i, j, value)
      end
   end
end

function TerrainGenerator:_copy_forced_edge_values(micro_map, zones, x, y)
   if zones == nil then return end

   local width = micro_map.width
   local height = micro_map.height
   local adj_map

   -- left zone
   adj_map = self:_get_generated_zone(zones, x-1, y)
   if adj_map then
      adj_map:copy_block(micro_map, adj_map, 1, 1, width, 1, 1, height)
   end

   -- right zone
   adj_map = self:_get_generated_zone(zones, x+1, y)
   if adj_map then
      adj_map:copy_block(micro_map, adj_map, width, 1, 1, 1, 1, height)
   end

   -- top zone
   adj_map = self:_get_generated_zone(zones, x, y-1)
   if adj_map then
      adj_map:copy_block(micro_map, adj_map, 1, 1, 1, height, width, 1)
   end

   -- bottom zone
   adj_map = self:_get_generated_zone(zones, x, y+1)
   if adj_map then
      adj_map:copy_block(micro_map, adj_map, 1, height, 1, 1, width, 1)
   end
end

function TerrainGenerator:_get_zone(zones, x, y)
   if x < 1 then return nil end
   if y < 1 then return nil end
   if x > zones.width then return nil end
   if y > zones.height then return nil end
   return zones:get(x, y)
end

function TerrainGenerator:_get_generated_zone(zones, x, y)
   local zone = self:_get_zone(zones, x, y)
   if zone == nil then return nil end
   if not zone.generated then return nil end
   return zone
end

function TerrainGenerator:_get_terrain_type(zones, x, y)
   local zone = self:_get_zone(zones, x, y)
   if zone == nil then return nil end
   return zone.terrain_type
end

function TerrainGenerator:_filter_noise_map(noise_map)
   local terrain_type = noise_map.terrain_type
   local width = noise_map.width
   local height = noise_map.height
   local filtered_map = HeightMap(width, height)

   filtered_map.terrain_type = terrain_type
   FilterFns.filter_2D_025(filtered_map, noise_map, width, height)
   filtered_map.generated = true

   return filtered_map
end

function TerrainGenerator:_quantize_micro_map(micro_map)
   -- non-uniform quantizer is not for the micro_map
   self:_quantize_height_map(micro_map, false)
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

   -- uncomment if edge tiles should not be eligible
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

function TerrainGenerator:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   oversize_map.terrain_type = micro_map.terrain_type

   -- micro_map should already be quantized by this point
   -- otherwise quantize before wavelet shaping
   for j=1, micro_height do
      for i=1, micro_width do
         value = micro_map:get(i, j)
         oversize_map:set_block((i-1)*self.tile_size+1, (j-1)*self.tile_size+1,
            self.tile_size, self.tile_size, value)
      end
   end
end

function TerrainGenerator:_shape_height_map(height_map, freq_scaling_coeff, levels)
   local width = height_map.width
   local height = height_map.height

   Wavelet.DWT_2D(height_map, width, height, levels)
   WaveletFns.scale_high_freq(height_map, width, height, freq_scaling_coeff, levels)
   self:_yield()

   Wavelet.IDWT_2D(height_map, width, height, levels)
   self:_yield()
end

function TerrainGenerator:_extract_zone_map(oversize_map)
   local zone_map_origin = self.tile_size/2 + 1
   local zone_map = HeightMap(self.zone_size, self.zone_size)

   zone_map.terrain_type = oversize_map.terrain_type

   oversize_map:copy_block(zone_map, oversize_map,
      1, 1, zone_map_origin, zone_map_origin, self.zone_size, self.zone_size)

   return zone_map
end

function TerrainGenerator:_quantize_height_map(height_map, enable_nonuniform_quantizer)
   local terrain_info = self.terrain_info
   local terrain_type = height_map.terrain_type
   local zone_min_height = terrain_info[terrain_type].min_height

   height_map:process_map(
      function (value)
         -- BUG: this makes quantizer different across zones,
         --      so this can cause edge tiles not to line up across zones
         if value <= zone_min_height then return zone_min_height end

         -- step_size depends on altitude and zone type
         local step_size = self:_get_step_size(value)
         local quantized_value = MathFns.quantize(value, step_size)
         if not enable_nonuniform_quantizer then return quantized_value end

         -- disable fancy mode for Plains
         if quantized_value <= terrain_info[TerrainType.Plains].max_height then
            return quantized_value
         end

         -- non-uniform quantizer
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

function TerrainGenerator:_get_step_size(value)
   local terrain_info = self.terrain_info

   if value > terrain_info[TerrainType.Foothills].max_height then
      return terrain_info[TerrainType.Mountains].step_size
   end

   if value > terrain_info[TerrainType.Plains].max_height then
      return terrain_info[TerrainType.Foothills].step_size
   end

   return terrain_info[TerrainType.Plains].step_size
end

function TerrainGenerator:_add_additional_details(zone_map)
   self._edge_detailer:add_detail_blocks(zone_map)

   -- handle plains separately - don't screw up edge detailer code
   self._edge_detailer:add_plains_details(zone_map)
end

function _print_blend_map(blend_map)
   local i, j, tile, str

   for j=1, blend_map.height do
      str = ''
      for i=1, blend_map.width do
         tile = blend_map:get(i, j)
         if tile == nil then
            str = str .. '   nil    '
         elseif tile.forced_value then
            str = str .. string.format('  (%4.1f)  ', tile.forced_value)
         else
            str = str .. ' ' .. string.format('%4.1f/%4.1f' , tile.mean, tile.std_dev)
         end
      end
      radiant.log.info(str)
   end
end

----------

function TerrainGenerator:_create_test_map(height_map)
   height_map:clear(8)
   height_map:set_block(192, 192, 32, 32, 16)
end

return TerrainGenerator

