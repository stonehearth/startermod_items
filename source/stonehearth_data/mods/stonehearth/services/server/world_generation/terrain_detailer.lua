local TerrainType = require 'services.server.world_generation.terrain_type'
local TerrainInfo = require 'services.server.world_generation.terrain_info'
local Array2D = require 'services.server.world_generation.array_2D'
local MathFns = require 'services.server.world_generation.math.math_fns'
local FilterFns = require 'services.server.world_generation.filter.filter_fns'
local InverseGaussianRandom = require 'services.server.world_generation.math.inverse_gaussian_random'
local TerrainDetailer = class()

-- note that the tile_width and tile_height passed in are currently the oversize width and height
function TerrainDetailer:__init(terrain_info, tile_width, tile_height, rng)
   self._terrain_info = terrain_info
   self._tile_width = tile_width
   self._tile_height = tile_height
   self._detail_seed_probability = 0.10
   self._detail_grow_probability = 0.85
   self._edge_threshold = 4
   self._rng = rng
   self._inverse_gaussian_random = InverseGaussianRandom(self._rng)

   self._edge_map_buffer = Array2D(self._tile_width, self._tile_height)
end

local function _angle_to_xy(angle)
   if angle ==   0 then return  1,  0 end
   if angle ==  90 then return  0,  1 end
   if angle == 180 then return -1,  0 end
   if angle == 270 then return  0, -1 end
   return nil, nil
end

local function _rotate_90(x, y)
   return -y, x
end

function TerrainDetailer:add_detail_blocks(tile_map)
   local rng = self._rng
   local edge
   local edge_threshold = self._edge_threshold
   local edge_map = self._edge_map_buffer
   local detail_seeds = {}
   local num_seeds = 0

   for j=1, tile_map.height do
      for i=1, tile_map.width do
         edge = self:_is_edge(tile_map, i, j, edge_threshold)
         edge_map:set(i, j, edge)

         if edge then
            if rng:get_real(0, 1) < self._detail_seed_probability then
               num_seeds = num_seeds + 1
               -- use a lua type instead of a C type so luajit can extremely agressively optimze it.
               detail_seeds[num_seeds] = { x = i, y = j }
            end
         end
      end
   end

   local point
   for i=1, num_seeds do
      point = detail_seeds[i]
      self:_grow_seed(tile_map, edge_map, point.x, point.y)
   end
end

function TerrainDetailer:_grow_seed(tile_map, edge_map, x, y)
   local edge = edge_map:get(x, y)
   if edge == false then return end

   local i, j, continue, base_height, detail_height

   base_height = tile_map:get(x, y)

   -- if edge is not false, edge is the delta to the highest neighbor
   detail_height = self:_generate_detail_height(edge, base_height)

   tile_map:set(x, y, base_height+detail_height)
   edge_map:set(x, y, false)

   i, j = x, y
   while true do
      -- grow left
      i = i - 1
      if i < 1 then break end
      continue = self:_try_grow(tile_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i, j = x, y
   while true do
      -- grow right
      i = i + 1
      if i > tile_map.width then break end
      continue = self:_try_grow(tile_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i, j = x, y
   while true do
      -- grow up
      j = j - 1
      if j < 1 then break end
      continue = self:_try_grow(tile_map, edge_map, i, j, detail_height)
      if not continue then break end
   end

   i, j = x, y
   while true do
      -- grow down
      j = j + 1
      if j > tile_map.height then break end
      continue = self:_try_grow(tile_map, edge_map, i, j, detail_height)
      if not continue then break end
   end
end

-- inverse bell curve from 1 to quantization size
function TerrainDetailer:_generate_detail_height(max_delta, base_height)
   if base_height >= self._terrain_info[TerrainType.foothills].max_height then
      -- if rng:get_real(0, 1) <= 0.50 then
         return max_delta
      -- else
         --return MathFns.round(max_delta*0.5)
      -- end
   else
      -- place the midpoint 2 standard deviations away
      -- edge values about 4x more likely than center value
      -- expanded form: ((max_delta+0.5) - (1-0.5)) / 2 / 2
      local std_dev = max_delta*0.25
      return self._inverse_gaussian_random:get_int(1, max_delta, std_dev)
   end
end

function TerrainDetailer:_generate_detail_height_uniform(max_delta)
   return self._rng:get_int(1, max_delta)
end

function TerrainDetailer:_try_grow(tile_map, edge_map, x, y, detail_height)
   local rng = self._rng
   local edge, value

   edge = edge_map:get(x, y)
   if edge == false then return false end
   if rng:get_real(0, 1) >= self._detail_grow_probability then return false end

   if edge < detail_height then detail_height = edge end

   value = tile_map:get(x, y)
   tile_map:set(x, y, value+detail_height)
   edge_map:set(x, y, false)
   return true
end

-- returns false is no non-diagonal neighbor higher by more then threshold
-- othrewise returns the height delta to the highest neighbor
function TerrainDetailer:_is_edge(tile_map, x, y, threshold)
   local neighbor
   local offset = tile_map:get_offset(x, y)
   local value = tile_map[offset]
   local width = tile_map.width
   local height = tile_map.height
   local delta, max_delta

   max_delta = threshold

   if x > 1 then
      neighbor = tile_map[offset-1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if x < width then
      neighbor = tile_map[offset+1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y > 1 then
      neighbor = tile_map[offset-width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y < height then
      neighbor = tile_map[offset+width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end

   if max_delta > threshold then
      return max_delta
   else
      return false
   end
end

-- makes lots of assumptions about how plains are quantized
-- ok since this will change anyway if plains are quantized differently
function TerrainDetailer:add_plains_details(tile_map)
   local edge_threshold = 2

   for j=1, tile_map.height do
      for i=1, tile_map.width do
         if self:_is_plains_edge(tile_map, i, j, edge_threshold) then
            local offset = tile_map:get_offset(i, j)
            tile_map[offset] = tile_map[offset] + 1
         end
      end
   end
end

function TerrainDetailer:_is_plains_edge(tile_map, x, y, threshold)
   local offset = tile_map:get_offset(x, y)
   local value = tile_map[offset]
   local width = tile_map.width
   local height = tile_map.height
   local neighbor

   if value >= self._terrain_info[TerrainType.plains].max_height then
      return false
   end

   if x > 1 then
      neighbor = tile_map[offset-1]
      if neighbor - value == threshold then return true end
   end
   if x < width then
      neighbor = tile_map[offset+1]
      if neighbor - value == threshold then return true end
   end
   if y > 1 then
      neighbor = tile_map[offset-width]
      if neighbor - value == threshold then return true end
   end
   if y < height then
      neighbor = tile_map[offset+width]
      if neighbor - value == threshold then return true end
   end

   return false
end

function TerrainDetailer:remove_mountain_chunks(tile_map, micro_map)
   local rng = self._rng
   local chunk_probability = 0.4
   local foothills_max_height = self._terrain_info[TerrainType.foothills].max_height
   local height, removed

   -- TODO: resolve chunks on edge macro_blocks
   for j=1, micro_map.height do
      for i=1, micro_map.width do
         height = micro_map:get(i, j)

         if height > foothills_max_height and rng:get_real(0, 1) <= chunk_probability then
            local dir, removed, angle, dx, dy
            
            -- randomly pick direction so chunks are not biased
            angle = 90*rng:get_int(0, 3)
            dx, dy = _angle_to_xy(angle)

            -- check all 4 directions unless chunk is removed
            for dir=1, 4 do
               removed = self:_remove_chunk(tile_map, micro_map, i, j, dx, dy)
               if removed then break end
               dx, dy = _rotate_90(dx, dy)
            end
         end
      end
   end
end

function TerrainDetailer:_remove_chunk(tile_map, micro_map, x, y, dx, dy)
   local mountains_step_size = self._terrain_info[TerrainType.mountains].step_size
   local macro_block_size = self._terrain_info.macro_block_size
   local height, adj_height
   local macro_block_x, macro_block_y, chunk_x, chunk_y
   local chunk_length, chunk_offset, chunk_depth

   height = micro_map:get(x, y)

   local adj_x = x + dx
   local adj_y = y + dy

   if micro_map:in_bounds(adj_x, adj_y) then
      adj_height = micro_map:get(adj_x, adj_y)
      if height - adj_height < mountains_step_size then
         return false
      end
   end

   macro_block_x = (x-1)*macro_block_size+1
   macro_block_y = (y-1)*macro_block_size+1

   chunk_length, chunk_offset = self:_generate_chunk_length_and_offset(macro_block_size)
   chunk_depth = mountains_step_size * 0.5

   if dy == -1 then
      -- left, (dx, dy) == (-1, 0)
      chunk_x = macro_block_x + chunk_offset
      chunk_y = macro_block_y
   elseif dy == 1 then
      -- right, (dx, dy) == (1, 0)
      chunk_x = macro_block_x + chunk_offset
      chunk_y = macro_block_y + macro_block_size - chunk_depth
   elseif dx == -1 then
      -- top, (dx, dy) == (0, -1)
      chunk_x = macro_block_x
      chunk_y = macro_block_y + chunk_offset
   else
      -- bottom, (dx, dy) == (0, 1)
      chunk_x = macro_block_x + macro_block_size - chunk_depth
      chunk_y = macro_block_y + chunk_offset
   end

   local new_height = height - chunk_depth
   local chunk_width, chunk_height -- as viewed from overhead

   if dx == 0 then
      chunk_width  = chunk_length
      chunk_height = chunk_depth
   else
      chunk_width  = chunk_depth
      chunk_height = chunk_length
   end

   tile_map:process_block(chunk_x, chunk_y, chunk_width, chunk_height,
      function (value)
         if value > new_height then return new_height end
         return value
      end
   )

   return true
end

function TerrainDetailer:_generate_chunk_length_and_offset(macro_block_size)
   local rng = self._rng
   local quarter_macro_block_size = macro_block_size * 0.25
   local chunk_length = quarter_macro_block_size * rng:get_int(2, 4)
   local chunk_offset = (macro_block_size - chunk_length) * rng:get_int(0, 1)
   return chunk_length, chunk_offset
end

return TerrainDetailer
