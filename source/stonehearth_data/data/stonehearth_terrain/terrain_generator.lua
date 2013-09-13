local TerrainGenerator = class()

local ZoneType = radiant.mods.require('/stonehearth_terrain/zone_type.lua')
local Array2D = radiant.mods.require('/stonehearth_terrain/array_2D.lua')
local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
local MathFns = radiant.mods.require('/stonehearth_terrain/math/math_fns.lua')
local FilterFns = radiant.mods.require('/stonehearth_terrain/filter/filter_fns.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local WaveletFns = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet_fns.lua')
local EdgeDetailer = radiant.mods.require('/stonehearth_terrain/edge_detailer.lua')

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (foothills, plains, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init()
   local zone_type, zone_params
   math.randomseed(3)

   self.zone_size = 256
   self.tile_size = 32

   self.wavelet_levels = 4
   self.frequency_scaling_coeff = 0.7

   local base_step_size = 8
   self.zone_params = {}
   zone_params = self.zone_params

   zone_type = ZoneType.Plains
   zone_params[zone_type] = {}
   zone_params[zone_type].step_size = 2
   zone_params[zone_type].mean_height = base_step_size
   zone_params[zone_type].std_dev = 10
   zone_params[zone_type].max_height = 8

   zone_type = ZoneType.Foothills
   zone_params[zone_type] = {}
   zone_params[zone_type].step_size = base_step_size
   zone_params[zone_type].mean_height = 24
   zone_params[zone_type].std_dev = 32
   zone_params[zone_type].max_height = 32

   zone_type = ZoneType.Mountains
   zone_params[zone_type] = {}
   zone_params[zone_type].step_size = base_step_size * 2
   zone_params[zone_type].mean_height = 48
   zone_params[zone_type].std_dev = 96

   zone_params.tree_line = zone_params[ZoneType.Foothills].max_height +
      zone_params[ZoneType.Mountains].step_size

   local oversize_zone_size = self.zone_size + self.tile_size
   self.oversize_map_buffer = HeightMap(oversize_zone_size, oversize_zone_size)

   self._edge_detailer = EdgeDetailer()
end

function TerrainGenerator:generate_zone(zone_type, zones, x, y)
   local zone_map, micro_map, noise_map
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

   self:_quantize_height_map(oversize_map, true)
   --self:_quantize_height_map(oversize_map, false)

   self._edge_detailer:add_detail_blocks(oversize_map, zone_type, self.zone_params)

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

   self:_quantize_height_map(filtered_map, false)

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

function TerrainGenerator:_quantize_height_map(height_map, fancy_mode)
   local zone_params = self.zone_params
   local zone_type = height_map.zone_type

   height_map:process_map(
      function (value)
         -- step_size depends on altitude and zone type
         local step_size = self:_get_step_size(zone_type, value)
         if value <= step_size then return step_size end

         local quantized_value = MathFns.quantize(value, step_size)
         if not fancy_mode then return quantized_value end

         -- disable fancy mode for Plains?
         --if value < 8 then return quantized_value end

         -- fancy mode
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

function TerrainGenerator:_get_step_size(zone_type, value)
   local zone_params = self.zone_params

   if value > zone_params[ZoneType.Foothills].max_height then
      return zone_params[ZoneType.Mountains].step_size
   end

   if value > zone_params[ZoneType.Plains].max_height then
      return zone_params[ZoneType.Foothills].step_size
   end

   if zone_type == ZoneType.Plains then
      return zone_params[ZoneType.Plains].step_size
   else
      return zone_params[ZoneType.Foothills].step_size
   end
end

----------

function TerrainGenerator:_create_test_map(height_map)
   height_map:clear(8)
   height_map:set_block(192, 192, 32, 32, 16)
end

return TerrainGenerator
