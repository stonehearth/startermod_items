local TerrainGenerator = class()

local TerrainType = radiant.mods.require('/stonehearth_terrain/terrain_type.lua')
local Array2D = radiant.mods.require('/stonehearth_terrain/array_2D.lua')
local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
local MathFns = radiant.mods.require('/stonehearth_terrain/math/math_fns.lua')
local FilterFns = radiant.mods.require('/stonehearth_terrain/filter/filter_fns.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local WaveletFns = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet_fns.lua')
local EdgeDetailer = radiant.mods.require('/stonehearth_terrain/edge_detailer.lua')
local TileInfo = radiant.mods.require('/stonehearth_terrain/tile_info.lua')
local Timer = radiant.mods.require('/stonehearth_debugtools/timer.lua')

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (foothills, plains, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init()
   local terrain_type, terrain_info

   self.base_random_seed = 3

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

function TerrainGenerator:_set_random_seed(x, y)
   local seed = self.base_random_seed
   if x ~= nil and y ~= nil then 
      seed = seed + y*10000 + x*100
   end
   math.randomseed(seed)
end

function TerrainGenerator:generate_zone(terrain_type, zones, x, y)
   local zone_map, micro_map, noise_map, blend_map
   local noise_map = self.noise_map_buffer
   local oversize_map = self.oversize_map_buffer
   local micro_width = oversize_map.width / self.tile_size
   local micro_height = oversize_map.height / self.tile_size
   local timer = Timer(Timer.CPU_TIME)

   if zones ~= nil then
      radiant.log.info('Generating zone %d, %d', x, y)
   end
   self:_set_random_seed(x, y)
   timer:start()

   noise_map.terrain_type = terrain_type
   oversize_map.terrain_type = terrain_type

   blend_map = self:_create_blend_map(micro_width, micro_height,
      terrain_type, zones, x, y)
   self:_fill_noise_map(noise_map, blend_map)

   -- micro_map must be a new HeightMap as it is returned from the function
   micro_map = self:_filter_and_quantize_noise_map(noise_map)

   -- force shared tiles to same value from adjacent zones
   self:_copy_forced_edge_values(micro_map, zones, x, y)

   self:_postprocess_micro_map(micro_map)

   self:_create_macro_map_from_micro_map(oversize_map, micro_map)

   -- copy the offset zone map from the oversize map
   -- zone_map must be a new HeightMap as it is returned from the function
   zone_map = self:_extract_zone_map(oversize_map)
   
   self:_shape_height_map(zone_map)

   -- enable non-uniform quantizer within a terrain type
   -- generates additional edge details
   self:_quantize_height_map(zone_map, true)
   
   self._edge_detailer:add_detail_blocks(zone_map)

   timer:stop()
   radiant.log.info('Zone generation time: %f', timer:seconds())

   return zone_map, micro_map
end

function TerrainGenerator:_create_blend_map(width, height, terrain_type, zones, x, y)
   local i, j, adjacent_zone, tile
   local terrain_mean = self.terrain_info[terrain_type].mean_height
   local terrain_std_dev = self.terrain_info[terrain_type].std_dev
   local blend_map = Array2D(width, height)
   blend_map.terrain_type = terrain_type

   -- default all tiles to the desired terrain_type for this zone
   for j=1, height do
      for i=1, width do
         tile = TileInfo()
         tile.mean = terrain_mean
         tile.std_dev = terrain_std_dev
         tile.mean_weight = 1
         tile.std_dev_weight = 1
         blend_map:set(i, j, tile)
      end
   end

   if zones == nil then return blend_map end

   -- blend values with left zone
   adjacent_zone = self:_get_zone(zones, x-1, y)
   self:_fill_blend_map(blend_map, adjacent_zone, height,      1,  1,  width, 1)

   -- blend values with right zone
   adjacent_zone = self:_get_zone(zones, x+1, y)
   self:_fill_blend_map(blend_map, adjacent_zone, height,  width, -1,      1, 1)

   -- blend values with top zone
   adjacent_zone = self:_get_zone(zones, x, y-1)
   self:_fill_blend_map(blend_map, adjacent_zone,  width,      1,  1, height, 2)

   -- blend values with bottom zone
   adjacent_zone = self:_get_zone(zones, x, y+1)
   self:_fill_blend_map(blend_map, adjacent_zone,  width, height, -1,      1, 2)

   -- avoid extreme values on corners by halving std_dev
   _halve_tile_std_dev(blend_map:get(1, 1))
   _halve_tile_std_dev(blend_map:get(width, 1))
   _halve_tile_std_dev(blend_map:get(1,     height))
   _halve_tile_std_dev(blend_map:get(width, height))

   radiant.log.info('Blend map:')
   _print_blend_map(blend_map)

   return blend_map
end

function _halve_tile_std_dev(tile)
   if tile.std_dev then tile.std_dev = tile.std_dev * 0.5 end
end

function _get_coords(x, y, orientation)
   if orientation == 1 then return x, y
   else                     return y, x end
end

function TerrainGenerator:_fill_blend_map(blend_map, adj_zone,
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
         cur_tile.mean_weight = 3
         cur_tile.std_dev_weight = 3
      end
      x, y = _get_coords(edge_index, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)

      -- row/column next to edge
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
         cur_tile.mean_weight = 1
         cur_tile.std_dev_weight = 1
      end
      x, y = _get_coords(edge_index+inc, i, orientation)
      blend_map:get(x, y):blend_stats_from(cur_tile)

      -- row/column two from edge
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
         cur_tile.mean_weight = 0.33
         cur_tile.std_dev_weight = 0.33
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

function _print_blend_map(blend_map)
   local i, j, tile, str

   for j=1, blend_map.height do
      str = ''
      for i=1, blend_map.width do
         tile = blend_map:get(i, j)
         if tile == nil then
            str = str .. '   nil    '
         elseif tile.forced_value then
            str = str .. '  forced  '
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
