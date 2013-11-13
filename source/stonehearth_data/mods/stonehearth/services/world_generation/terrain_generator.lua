local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
local MathFns = require 'services.world_generation.math.math_fns'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local Wavelet = require 'services.world_generation.filter.wavelet'
local WaveletFns = require 'services.world_generation.filter.wavelet_fns'
local EdgeDetailer = require 'services.world_generation.edge_detailer'
local TileInfo = require 'services.world_generation.tile_info'
local Timer = require 'services.world_generation.timer'

local TerrainGenerator = class()

-- Definitions
-- Block = atomic unit of terrain
-- Tile = square unit of flat land, also the chunk size of what is sent to the renderer
-- Zone = composed of an array of tiles fitting a theme (grassland, foothills, mountains)
-- Biome = a collection of zones fitting a larger theme (tundra, desert)?
-- World = the entire playspace of a game

function TerrainGenerator:__init(async, seed)
   local default_seed = 3

   if async == nil then async = false end
   if seed == nil then seed = default_seed end

   self.async = async
   self.random_seed = seed

   self.zone_size = 256
   self.tile_size = 32

   self.wavelet_levels = 4
   self.frequency_scaling_coeff = 0.7

   self.terrain_info = TerrainInfo()

   local oversize_zone_size = self.zone_size + self.tile_size
   self.oversize_map_buffer = Array2D(oversize_zone_size, oversize_zone_size)

   local micro_size = oversize_zone_size / self.tile_size
   self.blend_map_buffer = self:_create_blend_map(micro_size, micro_size)
   self.noise_map_buffer = Array2D(micro_size, micro_size)

   self._edge_detailer = EdgeDetailer(self.terrain_info)
end

function TerrainGenerator:_set_random_seed(x, y)
   local max_unsigned_int = 4294967295
   local seed = self.random_seed

   if x ~= nil and y ~= nil then 
      seed = seed + MathFns.point_hash(x, y)
   end

   math.randomseed(seed % max_unsigned_int)
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
   local micro_map, zone_map
   local zone_timer = Timer(Timer.CPU_TIME)
   local micro_map_timer = Timer(Timer.CPU_TIME)

   if zones ~= nil then
      radiant.log.info('Generating zone %d, %d', x, y)
   end

   -- make each zone deterministic on x, y so we can modify a zone without affecting othes
   self:_set_random_seed(x, y)

   zone_timer:start()
   micro_map_timer:start()

   micro_map = self:_create_micro_map(terrain_type, zones, x, y)
   micro_map_timer:stop()
   --radiant.log.info('Micromap generation time: %.3fs', micro_map_timer:seconds())

   zone_map = self:_create_zone_map(micro_map)
   zone_timer:stop()
   radiant.log.info('Zone generation time: %.3fs', zone_timer:seconds())

   return zone_map, micro_map
end

function TerrainGenerator:_create_micro_map(terrain_type, zones, x, y)
   local blend_map = self.blend_map_buffer
   local noise_map = self.noise_map_buffer
   local micro_map

   blend_map.terrain_type = terrain_type
   noise_map.terrain_type = terrain_type

   self:_fill_blend_map(blend_map, zones, x, y)
   --radiant.log.info('Blend map:'); _print_blend_map(blend_map)

   self:_fill_noise_map(noise_map, blend_map)
   --radiant.log.info('Noise map:'); noise_map:print()

   micro_map = self:_filter_noise_map(noise_map)
   --radiant.log.info('Filtered Noise map:'); micro_map:print()

   --self:_consolidate_mountain_blocks(micro_map, zones, x, y)
   --radiant.log.info('Consolidated Micro map:'); micro_map:print()

   self:_quantize_height_map(micro_map, true)
   --radiant.log.info('Quantized Micro map:'); micro_map:print()

   self:_copy_forced_edge_values(micro_map, zones, x, y)
   --radiant.log.info('Forced edge Micro map:'); micro_map:print()

   self:_postprocess_micro_map(micro_map)
   --radiant.log.info('Postprocessed Micro map:'); micro_map:print()

   -- copy edges again in case postprocessing changed them
   self:_copy_forced_edge_values(micro_map, zones, x, y)
   --radiant.log.info('Final Micro map:'); micro_map:print()

   self:_yield()

   return micro_map
end

function TerrainGenerator:_create_zone_map(micro_map)
   local oversize_map = self.oversize_map_buffer
   local zone_map

   self:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   self:_yield()

   self:_shape_height_map(oversize_map, self.frequency_scaling_coeff, self.wavelet_levels) -- CHECKCHECK
   self:_yield()

   self:_quantize_height_map(oversize_map, false)
   self:_yield()

   self:_add_additional_details(oversize_map, micro_map)
   self:_yield()

   -- copy the offset zone map from the oversize map
   zone_map = self:_extract_zone_map(oversize_map)
   self:_yield()

   return zone_map
end

-- allows this long running job to be completed in multiple sessions
function TerrainGenerator:_yield()
   if self.async then
      coroutine.yield()
   end
end

function TerrainGenerator:_fill_blend_map(blend_map, zones, x, y)
   local i, j, adj_zone, tile
   local width = blend_map.width
   local height = blend_map.height
   local terrain_type = blend_map.terrain_type
   local terrain_mean = self.terrain_info[terrain_type].mean_height
   local terrain_std_dev = self.terrain_info[terrain_type].std_dev

   if terrain_type == TerrainType.Mountains and
      self:_surrounded_by_terrain(TerrainType.Mountains, zones, x, y) then
      terrain_mean = terrain_mean + self.terrain_info[TerrainType.Mountains].step_size
   end

   if terrain_type == TerrainType.Grassland and
      self:_surrounded_by_terrain(TerrainType.Grassland, zones, x, y) then
      terrain_mean = terrain_mean - self.terrain_info[TerrainType.Grassland].step_size
      assert(terrain_mean >= self.terrain_info[TerrainType.Grassland].min_height)
   end

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
      adj_zone = self:_get_zone(zones, x-1, y)
      self:_blend_zone(blend_map, adj_zone, height,      1,  1,  width, 1)

      -- blend values with right zone
      adj_zone = self:_get_zone(zones, x+1, y)
      self:_blend_zone(blend_map, adj_zone, height,  width, -1,      1, 1)

      -- blend values with top zone
      adj_zone = self:_get_zone(zones, x, y-1)
      self:_blend_zone(blend_map, adj_zone,  width,      1,  1, height, 2)

      -- blend values with bottom zone
      adj_zone = self:_get_zone(zones, x, y+1)
      self:_blend_zone(blend_map, adj_zone,  width, height, -1,      1, 2)
   end

   -- avoid extreme values along edges by halving std_dev
   -- for j=1, height do
   --    for i=1, width do
   --       if blend_map:is_boundary(i, j) then
   --          _halve_tile_std_dev(blend_map:get(i, j)) -- CHECKCHECK
   --       end
   --    end
   -- end

   return blend_map
end

function TerrainGenerator:_surrounded_by_terrain(terrain_type, zones, x, y)
   local zone

   zone = self:_get_zone(zones, x-1, y)
   if zone ~= nil and zone.terrain_type ~= terrain_type then return false end

   zone = self:_get_zone(zones, x+1, y)
   if zone ~= nil and zone.terrain_type ~= terrain_type then return false end

   zone = self:_get_zone(zones, x, y-1)
   if zone ~= nil and zone.terrain_type ~= terrain_type then return false end

   zone = self:_get_zone(zones, x, y+1)
   if zone ~= nil and zone.terrain_type ~= terrain_type then return false end

   return true
end

function _halve_tile_std_dev(tile)
   if tile.std_dev then tile.std_dev = tile.std_dev * 0.5 end
end

function _get_coords(x, y, orientation)
   if orientation == 1 then return x, y
   else                     return y, x end
end

--local blend_filter             = { [0] = 1.00,  0.33, 0.00 }
--local forced_edge_blend_filter = { [0] =  nil, 10.00, 1.00 }
--local blend_filter             = { [0] = 1.0, 0.5, 0.2,   0 }
--local forced_edge_blend_filter = { [0] = nil, 10.0, 1.0, 0.33 }
-- 8th order, 0.25 cutoff, normalized to 50/50 average on boundary
local blend_filter             = { [0] = 1.0000, 0.7797, 0.3451, 0.0653, 0.0003 }

-- 8th order, 0.25 cutoff, normalized to 75/25 on row next to boundary
-- needs stronger weight becuase adjacent zone cannot be filtered (it is already generated)
local forced_edge_blend_filter = { [0] =    nil, 3.0000, 2.3390, 1.0354, 0.1959 }


function TerrainGenerator:_blend_zone(blend_map, adj_zone,
   edge_length, edge_index, inc, adj_edge_index, orientation)
   if adj_zone == nil then return end

   local terrain_info = self.terrain_info
   local cur_terrain_type = blend_map.terrain_type
   local cur_mean = terrain_info[cur_terrain_type].mean_height
   local adj_terrain_type = adj_zone.terrain_type
   local adj_mean = terrain_info[adj_terrain_type].mean_height
   local adj_std_dev = terrain_info[adj_terrain_type].std_dev
   local cur_tile = TileInfo()
   local i, d, edge_value, edge_std_dev, x, y

   assert(#blend_filter == #forced_edge_blend_filter)
   assert((#blend_filter+1 <= blend_map.width) and (#blend_filter+1 <= blend_map.height))

   for i=1, edge_length do
      for d=0, #blend_filter do
         cur_tile:clear()

         if adj_zone.generated then
            if d == 0 then
               -- force edge values since they are shared
               x, y = _get_coords(adj_edge_index, i, orientation)
               edge_value = adj_zone:get(x, y)
               edge_std_dev = self:_calc_std_dev(edge_value)
               cur_tile.forced_value = edge_value
               -- value is forced, no need to set std_dev
            else
               cur_tile.mean = edge_value
               if d <= 1 then
                  cur_tile.std_dev = 0 -- CHECKCHECK
               else
                  cur_tile.std_dev = edge_std_dev
               end
               cur_tile.mean_weight = forced_edge_blend_filter[d]
               cur_tile.std_dev_weight = forced_edge_blend_filter[d]
            end
         else
            cur_tile.mean = adj_mean
            cur_tile.std_dev = adj_std_dev
            cur_tile.mean_weight = blend_filter[d] -- should be ready to refactor both weights
            cur_tile.std_dev_weight = blend_filter[d]
         end

         -- get tile that is distance d from edge
         -- edge index is 1 or width/height
         -- inc is +1 or -1
         -- i walks the edge at distance d
         -- orientation indicates horizontal or vertical edge
         x, y = _get_coords(edge_index+d*inc, i, orientation)

         -- add mean and std_dev components to the mix
         blend_map:get(x, y):blend_stats_from(cur_tile)
      end
   end
end

function TerrainGenerator:_calc_std_dev(height)
   local terrain_info = self.terrain_info
   local prev_mean_height, prev_std_dev
   local i, ti, value

   i = TerrainType.Grassland
   prev_mean_height = 0
   prev_std_dev = terrain_info[i].std_dev

   for i=TerrainType.Grassland, TerrainType.Mountains do
      ti = terrain_info[i]

      if height <= ti.mean_height then
         value = MathFns.interpolate(height, prev_mean_height, prev_std_dev, ti.mean_height, ti.std_dev)
         return value
      end

      prev_mean_height = ti.mean_height
      prev_std_dev = ti.std_dev
   end

   return ti.std_dev
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

function TerrainGenerator:_filter_noise_map(noise_map)
   local terrain_type = noise_map.terrain_type
   local width = noise_map.width
   local height = noise_map.height
   local filtered_map = Array2D(width, height)

   filtered_map.terrain_type = terrain_type
   FilterFns.filter_2D_025(filtered_map, noise_map, width, height, 8)
   filtered_map.generated = true

   return filtered_map

   -- local micro_map = Array2D(width, height) -- CHECKCHECK
   -- micro_map.terrain_type = terrain_type
   -- FilterFns.filter_max_slope(micro_map, filtered_map, width, height, 8)
   -- micro_map.generated = true

   -- return micro_map
end

function TerrainGenerator:_add_DC_component(micro_map, blend_map)
   local width = micro_map.width
   local height = micro_map.height
   local i, j, tile, mean, offset

   for j=1, height do
      for i=1, width do
         tile = blend_map:get(i, j)

         if tile.forced_value then mean = tile.forced_value
         else                      mean = tile.mean
         end

         offset = micro_map:get_offset(i, j)
         micro_map[offset] = micro_map[offset] + mean
      end
   end
end

-- edge values may not change values! they are shared with the adjacent zone
function TerrainGenerator:_postprocess_micro_map(micro_map)
   self:_fill_holes(micro_map)
end

function TerrainGenerator:_consolidate_mountain_blocks(micro_map, zones, x, y)
   local max_foothills_height = self.terrain_info[TerrainType.Foothills].max_height
   local start_x = 1
   local start_y = 1
   local i, j, value

   -- skip edges that have already been defined
   if self:_get_generated_zone(zones, x-1, y) ~= nil then
      start_x = 2
   end

   if self:_get_generated_zone(zones, x, y-1) ~= nil then
      start_y = 2
   end

   -- oppposite edge should not be defined, but if it is,
   --    it's ok since it will be overwritten by the forced edge values

   for j=start_y, micro_map.height, 2 do
      for i=start_x, micro_map.width, 2 do
         value = self:_average_quad(micro_map, i, j)
         if value > max_foothills_height then
            self:_set_quad(micro_map, i, j, value)
         end
      end
   end
end

function TerrainGenerator:_average_quad(micro_map, x, y)
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

function TerrainGenerator:_set_quad(micro_map, x, y, value)
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

function TerrainGenerator:_fill_holes(micro_map)
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
   if zones:in_bounds(x, y) then
      return zones:get(x, y)
   end
   return nil
end

function TerrainGenerator:_get_generated_zone(zones, x, y)
   local zone = self:_get_zone(zones, x, y)
   if zone == nil or not zone.generated then
      return nil
   end
   return zone
end

function TerrainGenerator:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   oversize_map.terrain_type = micro_map.terrain_type

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

   Wavelet.DWT_2D(height_map, width, height, levels, self.async)
   WaveletFns.scale_high_freq(height_map, width, height, freq_scaling_coeff, levels)
   self:_yield()
   Wavelet.IDWT_2D(height_map, width, height, levels, self.async)
end

function TerrainGenerator:_quantize_height_map(height_map, is_micro_map)
   local terrain_info = self.terrain_info
   local terrain_type = height_map.terrain_type
   local enable_fancy_quantizer = not is_micro_map
   local global_min_height = terrain_info[TerrainType.Grassland].min_height
   local recommended_min_height = terrain_info[terrain_type].min_height
   local i, j, offset, value, min_height, quantized_value

   for j=1, height_map.height do
      for i=1, height_map.width do
         offset = height_map:get_offset(i, j)
         value = height_map[offset]

         if false then -- CHECKCHECK
         --if is_micro_map and not height_map:is_boundary(i, j) then
            -- must relax height requirements on edges to match forced tiles
            -- don't have to do this for edges adjacent to zones that are not generated yet
            min_height = recommended_min_height
         else
            min_height = global_min_height
         end

         quantized_value = self:_quantize_value(value, min_height, enable_fancy_quantizer)
         height_map[offset] = quantized_value
      end
   end
end

function TerrainGenerator:_quantize_value(value, min_height, enable_fancy_quantizer)
   local terrain_info = self.terrain_info

   if value <= min_height then return min_height end

   -- step_size depends on altitude and zone type
   -- replace this with a real non-uniform quantizer
   local step_size = self:_get_step_size(value)
   local quantized_value = MathFns.quantize(value, step_size)
   if not enable_fancy_quantizer then return quantized_value end

   -- disable fancy mode for Grassland
   if quantized_value <= terrain_info[TerrainType.Grassland].max_height then
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

function TerrainGenerator:_get_step_size(value)
   local terrain_type = self.terrain_info:get_terrain_type(value)
   return self.terrain_info[terrain_type].step_size
end

function TerrainGenerator:_add_additional_details(height_map, micro_map)
   self._edge_detailer:remove_mountain_chunks(height_map, micro_map)

   self._edge_detailer:add_detail_blocks(height_map)

   -- handle grassland separately - don't screw up edge detailer code
   self._edge_detailer:add_grassland_details(height_map)
end

function TerrainGenerator:_extract_zone_map(oversize_map)
   local zone_map_origin = self.tile_size/2 + 1
   local zone_map = Array2D(self.zone_size, self.zone_size)

   zone_map.terrain_type = oversize_map.terrain_type

   oversize_map:copy_block(zone_map, oversize_map,
      1, 1, zone_map_origin, zone_map_origin, self.zone_size, self.zone_size)

   return zone_map
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

-----

function TerrainGenerator:_create_test_micro_map(micro_map)
   micro_map:clear(32)
   micro_map:set_block(4, 4, 1, 1, 64)
end

return TerrainGenerator
