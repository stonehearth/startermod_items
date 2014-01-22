local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local NonUniformQuantizer = require 'services.world_generation.math.non_uniform_quantizer'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local Wavelet = require 'services.world_generation.filter.wavelet'
local WaveletFns = require 'services.world_generation.filter.wavelet_fns'
local TerrainDetailer = require 'services.world_generation.terrain_detailer'
local MacroBlock = require 'services.world_generation.macro_block'
local Timer = require 'services.world_generation.timer'

local TerrainGenerator = class()
local log = radiant.log.create_logger('world_generation')

-- Definitions
-- Block = atomic unit of terrain that cannot be subdivided
-- MacroBlock = square unit of flat land, 32x32, but can shift a bit due to toplogy
-- Tile = 2D array of MacroBlocks fitting a theme (plains, foothills, mountains)
--        These 256x256 terrain tiles are different from nav grid tiles which are 16x16.
-- World = the entire playspace of a game

-- TODO: remove terrain_type from all the micro_maps
function TerrainGenerator:__init(rng, async)
   if async == nil then async = false end

   self._rng = rng
   self._async = async

   self.tile_size = 256
   self.macro_block_size = 32

   self._wavelet_levels = 4
   self._frequency_scaling_coeff = 0.7

   self.terrain_info = TerrainInfo()

   local oversize_tile_size = self.tile_size + self.macro_block_size
   self._oversize_map_buffer = Array2D(oversize_tile_size, oversize_tile_size)

   local micro_size = oversize_tile_size / self.macro_block_size
   self._blend_map_buffer = self:_create_blend_map(micro_size, micro_size)
   self._noise_map_buffer = Array2D(micro_size, micro_size)

   self._terrain_detailer = TerrainDetailer(self.terrain_info, oversize_tile_size, oversize_tile_size, self._rng)

   self:_initialize_quantizer()
end

function TerrainGenerator:_initialize_quantizer()
   local terrain_info = self.terrain_info
   local plains_info = terrain_info[TerrainType.plains]
   local foothills_info = terrain_info[TerrainType.foothills]
   local mountains_info = terrain_info[TerrainType.mountains]
   local centroids = {}
   local min, max, step_size
   
   min = terrain_info.min_height
   max = plains_info.max_height
   step_size = plains_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   min = plains_info.max_height + foothills_info.step_size
   max = foothills_info.max_height
   step_size = foothills_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   min = foothills_info.max_height + mountains_info.step_size
   max = min + mountains_info.step_size*10
   step_size = mountains_info.step_size
   for value = min, max, step_size do
      table.insert(centroids, value)
   end

   self._quantizer = NonUniformQuantizer(centroids)
end

function TerrainGenerator:_create_blend_map(width, height)
   local i, j
   local blend_map = Array2D(width, height)

   for j=1, width do
      for i=1, height do
         blend_map:set(i, j, MacroBlock())
      end
   end

   return blend_map
end

function TerrainGenerator:generate_tile(terrain_type, blueprint, x, y)
   local micro_map, tile_map
   local tile_timer = Timer(Timer.CPU_TIME)
   local micro_map_timer = Timer(Timer.CPU_TIME)

   if blueprint ~= nil then
      log:info('Generating tile %d, %d', x, y)
   end

   tile_timer:start()
   micro_map_timer:start()

   micro_map = self:_create_micro_map(terrain_type, blueprint, x, y)
   micro_map_timer:stop()
   log:info('Micromap generation time: %.3fs', micro_map_timer:seconds())

   tile_map = self:_create_tile_map(micro_map)
   tile_timer:stop()
   log:info('Tile generation time: %.3fs', tile_timer:seconds())

   return tile_map, micro_map
end

function TerrainGenerator:_create_micro_map(terrain_type, blueprint, x, y)
   local blend_map = self._blend_map_buffer
   local noise_map = self._noise_map_buffer
   local micro_map

   blend_map.terrain_type = terrain_type
   noise_map.terrain_type = terrain_type

   self:_fill_blend_map(blend_map, blueprint, x, y)
   -- log:debug('Blend map:'); _print_blend_map(blend_map)

   self:_fill_noise_map(noise_map, blend_map)
   -- log:debug('Noise map:'); noise_map:print()

   micro_map = self:_filter_noise_map(noise_map)
   -- log:debug('Filtered Noise map:'); micro_map:print()

   --self:_consolidate_mountain_blocks(micro_map, blueprint, x, y)
   -- log:debug('Consolidated Micro map:'); micro_map:print()

   self:_quantize_height_map(micro_map, true)
   -- log:debug('Quantized Micro map:'); micro_map:print()

   self:_copy_forced_edge_values(micro_map, blueprint, x, y)
   -- log:debug('Forced edge Micro map:'); micro_map:print()

   self:_postprocess_micro_map(micro_map)
   -- log:debug('Postprocessed Micro map:'); micro_map:print()

   -- copy edges again in case postprocessing changed them
   self:_copy_forced_edge_values(micro_map, blueprint, x, y)
   -- log:debug('Final Micro map:'); micro_map:print()

   self:_yield()

   return micro_map
end

function TerrainGenerator:_create_tile_map(micro_map)
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

-- TODO: replace this blend_map architecture
-- There is a hard to fix bug when estimating the blending values at the corners where 3 maps meet
-- Use a deterministic pseudo-random sequence to determine (and recreate if necessary) the pre-filtered
-- values of neighboring tiles to filter across boundaries instead of trying to extend a bandlimited signal
function TerrainGenerator:_fill_blend_map(blend_map, blueprint, x, y)
   local i, j, adj_tile_info, macro_block
   local width = blend_map.width
   local height = blend_map.height
   local terrain_type = blend_map.terrain_type
   local terrain_mean = self.terrain_info[terrain_type].mean_height
   local terrain_std_dev = self.terrain_info[terrain_type].std_dev

   if terrain_type == TerrainType.mountains and
      self:_surrounded_by_terrain(TerrainType.mountains, blueprint, x, y) then
      terrain_mean = terrain_mean + self.terrain_info[TerrainType.mountains].step_size
   end

   if terrain_type == TerrainType.plains and
      self:_surrounded_by_terrain(TerrainType.plains, blueprint, x, y) then
      terrain_mean = terrain_mean - self.terrain_info[TerrainType.plains].step_size
      assert(terrain_mean >= self.terrain_info.min_height)
   end

   for j=1, height do
      for i=1, width do
         macro_block = blend_map:get(i, j)
         macro_block:clear()
         macro_block.mean = terrain_mean
         macro_block.std_dev = terrain_std_dev
         macro_block.mean_weight = 1
         macro_block.std_dev_weight = 1
      end
   end

   if blueprint ~= nil then
      -- blend values with left tile
      adj_tile_info = self:_get_tile_info(blueprint, x-1, y)
      self:_blend_tile(blend_map, adj_tile_info, height,      1,  1,  width, 1)

      -- blend values with right tile
      adj_tile_info = self:_get_tile_info(blueprint, x+1, y)
      self:_blend_tile(blend_map, adj_tile_info, height,  width, -1,      1, 1)

      -- blend values with top tile
      adj_tile_info = self:_get_tile_info(blueprint, x, y-1)
      self:_blend_tile(blend_map, adj_tile_info,  width,      1,  1, height, 2)

      -- blend values with bottom tile
      adj_tile_info = self:_get_tile_info(blueprint, x, y+1)
      self:_blend_tile(blend_map, adj_tile_info,  width, height, -1,      1, 2)
   end

   -- avoid extreme values along edges by halving std_dev
   -- for j=1, height do
   --    for i=1, width do
   --       if blend_map:is_boundary(i, j) then
   --          _halve_macro_block_std_dev(blend_map:get(i, j))
   --       end
   --    end
   -- end

   return blend_map
end

function TerrainGenerator:_surrounded_by_terrain(terrain_type, blueprint, x, y)
   local tile_info

   tile_info = self:_get_tile_info(blueprint, x-1, y)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then return false end

   tile_info = self:_get_tile_info(blueprint, x+1, y)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then return false end

   tile_info = self:_get_tile_info(blueprint, x, y-1)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then return false end

   tile_info = self:_get_tile_info(blueprint, x, y+1)
   if tile_info ~= nil and tile_info.terrain_type ~= terrain_type then return false end

   return true
end

function _halve_macro_block_std_dev(macro_block)
   if macro_block.std_dev then macro_block.std_dev = macro_block.std_dev * 0.5 end
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
-- needs stronger weight becuase adjacent tile cannot be filtered (it is already generated)
local forced_edge_blend_filter = { [0] =    nil, 3.0000, 2.3390, 1.0354, 0.1959 }


function TerrainGenerator:_blend_tile(blend_map, adj_tile_info,
   edge_length, edge_index, inc, adj_edge_index, orientation)
   if adj_tile_info == nil then return end

   local terrain_info = self.terrain_info
   local cur_terrain_type = blend_map.terrain_type
   local cur_mean = terrain_info[cur_terrain_type].mean_height
   local adj_terrain_type = adj_tile_info.terrain_type
   local adj_mean = terrain_info[adj_terrain_type].mean_height
   local adj_std_dev = terrain_info[adj_terrain_type].std_dev
   local cur_macro_block = MacroBlock()
   local i, d, edge_value, edge_std_dev, x, y

   assert(#blend_filter == #forced_edge_blend_filter)
   assert((#blend_filter+1 <= blend_map.width) and (#blend_filter+1 <= blend_map.height))

   for i=1, edge_length do
      for d=0, #blend_filter do
         cur_macro_block:clear()

         if adj_tile_info.generated then
            if d == 0 then
               -- force edge values since they are shared
               x, y = _get_coords(adj_edge_index, i, orientation)
               edge_value = adj_tile_info.micro_map:get(x, y)
               edge_std_dev = self:_calc_std_dev(edge_value)
               cur_macro_block.forced_value = edge_value
               -- value is forced, no need to set std_dev
            else
               cur_macro_block.mean = edge_value
               if d <= 1 then
                  cur_macro_block.std_dev = 0 -- CHECKCHECK
               else
                  cur_macro_block.std_dev = edge_std_dev
               end
               cur_macro_block.mean_weight = forced_edge_blend_filter[d]
               cur_macro_block.std_dev_weight = forced_edge_blend_filter[d]
            end
         else
            cur_macro_block.mean = adj_mean
            cur_macro_block.std_dev = adj_std_dev
            cur_macro_block.mean_weight = blend_filter[d] -- should be ready to refactor both weights
            cur_macro_block.std_dev_weight = blend_filter[d]
         end

         -- get macro_block that is distance d from edge
         -- edge index is 1 or width/height
         -- inc is +1 or -1
         -- i walks the edge at distance d
         -- orientation indicates horizontal or vertical edge
         x, y = _get_coords(edge_index+d*inc, i, orientation)

         -- add mean and std_dev components to the mix
         blend_map:get(x, y):blend_stats_from(cur_macro_block)
      end
   end
end

function TerrainGenerator:_calc_std_dev(height)
   local terrain_info = self.terrain_info
   local terrain_order = terrain_info.terrain_order
   local prev_mean_height, prev_std_dev
   local ti, terrain_type, value

   terrain_type = terrain_order[1]
   prev_mean_height = 0
   prev_std_dev = terrain_info[terrain_type].std_dev

   for _, terrain_type in ipairs(terrain_order) do
      ti = terrain_info[terrain_type]

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
   local rng = self._rng

   local noise_fn = function(i, j)
      local value, macro_block

      macro_block = blend_map:get(i, j)
      if macro_block.forced_value then
         value = macro_block.forced_value
      else
         value = rng:get_gaussian(macro_block.mean, macro_block.std_dev)
      end
      return value
   end

   noise_map:fill(noise_fn)
end

-- must return a new micro_map each time
function TerrainGenerator:_filter_noise_map(noise_map)
   local terrain_type = noise_map.terrain_type
   local width = noise_map.width
   local height = noise_map.height
   local filtered_map = Array2D(width, height)

   filtered_map.terrain_type = terrain_type
   FilterFns.filter_2D_025(filtered_map, noise_map, width, height, 8)

   return filtered_map

   -- local micro_map = Array2D(width, height)
   -- micro_map.terrain_type = terrain_type
   -- FilterFns.filter_max_slope(micro_map, filtered_map, width, height, 8)
   -- micro_map.generated = true

   -- return micro_map
end

function TerrainGenerator:_add_DC_component(micro_map, blend_map)
   local width = micro_map.width
   local height = micro_map.height
   local i, j, macro_block, mean, offset

   for j=1, height do
      for i=1, width do
         macro_block = blend_map:get(i, j)

         if macro_block.forced_value then mean = macro_block.forced_value
         else                      mean = macro_block.mean
         end

         offset = micro_map:get_offset(i, j)
         micro_map[offset] = micro_map[offset] + mean
      end
   end
end

-- edge values may not change values! they are shared with the adjacent tile
function TerrainGenerator:_postprocess_micro_map(micro_map)
   self:_fill_holes(micro_map)
end

function TerrainGenerator:_consolidate_mountain_blocks(micro_map, blueprint, x, y)
   local max_foothills_height = self.terrain_info[TerrainType.foothills].max_height
   local start_x = 1
   local start_y = 1
   local i, j, value

   -- skip edges that have already been defined
   if self:_get_micro_map(blueprint, x-1, y) ~= nil then
      start_x = 2
   end

   if self:_get_micro_map(blueprint, x, y-1) ~= nil then
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

   -- uncomment if edge macro_blocks should not be eligible
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

function TerrainGenerator:_copy_forced_edge_values(micro_map, blueprint, x, y)
   if blueprint == nil then return end

   local width = micro_map.width
   local height = micro_map.height
   local adj_micro_map

   -- left tile
   adj_micro_map = self:_get_micro_map(blueprint, x-1, y)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, 1, width, 1, 1, height)
   end

   -- right tile
   adj_micro_map = self:_get_micro_map(blueprint, x+1, y)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, width, 1, 1, 1, 1, height)
   end

   -- top tile
   adj_micro_map = self:_get_micro_map(blueprint, x, y-1)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, 1, 1, height, width, 1)
   end

   -- bottom tile
   adj_micro_map = self:_get_micro_map(blueprint, x, y+1)
   if adj_micro_map then
      Array2D.copy_block(micro_map, adj_micro_map, 1, height, 1, 1, width, 1)
   end
end

function TerrainGenerator:_get_tile_info(blueprint, x, y)
   if blueprint:in_bounds(x, y) then
      return blueprint:get(x, y)
   end
   return nil
end

function TerrainGenerator:_get_micro_map(blueprint, x, y)
   local tile_info = self:_get_tile_info(blueprint, x, y)
   if tile_info == nil or not tile_info.generated then
      return nil
   end
   return tile_info.micro_map
end

function TerrainGenerator:_create_oversize_map_from_micro_map(oversize_map, micro_map)
   local i, j, value
   local micro_width = micro_map.width
   local micro_height = micro_map.height
   oversize_map.terrain_type = micro_map.terrain_type

   for j=1, micro_height do
      for i=1, micro_width do
         value = micro_map:get(i, j)
         oversize_map:set_block((i-1)*self.macro_block_size+1, (j-1)*self.macro_block_size+1,
            self.macro_block_size, self.macro_block_size, value)
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
   local terrain_info = self.terrain_info
   local terrain_type = height_map.terrain_type
   local enable_fancy_quantizer = not is_micro_map
   local i, j, offset, value, quantized_value

   for j=1, height_map.height do
      for i=1, height_map.width do
         offset = height_map:get_offset(i, j)
         value = height_map[offset]

         quantized_value = self:_quantize_value(value, enable_fancy_quantizer)
         height_map[offset] = quantized_value
      end
   end
end

function TerrainGenerator:_quantize_value(value, enable_fancy_quantizer)
   local quantized_value = self._quantizer:quantize(value)

   if not enable_fancy_quantizer then
      return quantized_value
   end

   local terrain_info = self.terrain_info
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

function TerrainGenerator:_add_additional_details(height_map, micro_map)
   self._terrain_detailer:remove_mountain_chunks(height_map, micro_map)

   self._terrain_detailer:add_detail_blocks(height_map)

   -- handle plains separately - don't screw up edge detailer code
   self._terrain_detailer:add_plains_details(height_map)
end

-- must return a new tile_map each time
function TerrainGenerator:_extract_tile_map(oversize_map)
   local tile_map_origin = self.macro_block_size/2 + 1
   local tile_map = Array2D(self.tile_size, self.tile_size)

   tile_map.terrain_type = oversize_map.terrain_type

   Array2D.copy_block(tile_map, oversize_map,
      1, 1, tile_map_origin, tile_map_origin, self.tile_size, self.tile_size)

   return tile_map
end

function TerrainGenerator:_yield()
   if self._async then
      coroutine.yield()
   end
end

function _print_blend_map(blend_map)
   local i, j, macro_block, str

   for j=1, blend_map.height do
      str = ''
      for i=1, blend_map.width do
         macro_block = blend_map:get(i, j)
         if macro_block == nil then
            str = str .. '   nil    '
         elseif macro_block.forced_value then
            str = str .. string.format('  (%4.1f)  ', macro_block.forced_value)
         else
            str = str .. ' ' .. string.format('%4.1f/%4.1f' , macro_block.mean, macro_block.std_dev)
         end
      end
      log:info(str)
   end
end

-----

function TerrainGenerator:_create_test_micro_map(micro_map)
   micro_map:clear(32)
   micro_map:set_block(4, 4, 1, 1, 64)
end

return TerrainGenerator
