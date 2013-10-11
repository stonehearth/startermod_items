local TerrainType = require 'services.world_generation.terrain_type'
local Array2D = require 'services.world_generation.array_2D'
local InverseGaussianRandom = require 'services.world_generation.math.inverse_gaussian_random'
local Point2 = _radiant.csg.Point2

local EdgeDetailer = class()

function EdgeDetailer:__init(terrain_info)
   self.terrain_info = terrain_info
   self.detail_seed_probability = 0.10
   self.detail_grow_probability = 0.85
end

function EdgeDetailer:add_detail_blocks(height_map)
   local i, j, edge
   local edge_threshold = 4
   local edge_map = Array2D(height_map.width, height_map.height)
   local detail_seeds = {}
   local num_seeds = 0

   for j=1, height_map.height do
      for i=1, height_map.width do
         edge = self:_is_edge(height_map, i, j, edge_threshold)
         edge_map:set(i, j, edge)

         if edge then
            if math.random() < self.detail_seed_probability then
               num_seeds = num_seeds + 1
               detail_seeds[num_seeds] = Point2(i, j)
            end
         end
      end
   end

   local point
   for i=1, num_seeds do
      point = detail_seeds[i]
      self:_grow_seed(height_map, edge_map, point.x, point.y)
   end
end

function EdgeDetailer:_grow_seed(height_map, edge_map, x, y)
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
function EdgeDetailer:_generate_detail_height(max_delta)
   -- place the midpoint 2 standard deviations away
   -- edge values about 4x more likely than center value
   -- expanded form: ((max_delta+0.5) - (1-0.5)) / 2 / 2
   local std_dev = max_delta*0.25
   return InverseGaussianRandom.generate_int(1, max_delta, std_dev)
end

function EdgeDetailer:_generate_detail_height_uniform(max_delta)
   return math.random(1, max_delta)
end

function EdgeDetailer:_try_grow(height_map, edge_map, x, y, detail_height)
   local edge, value

   edge = edge_map:get(x, y)
   if edge == false then return false end
   if math.random() >= self.detail_grow_probability then return false end

   if edge < detail_height then detail_height = edge end

   value = height_map:get(x, y)
   height_map:set(x, y, value+detail_height)
   edge_map:set(x, y, false)
   return true
end

-- returns false is no non-diagonal neighbor higher by more then threshold
-- othrewise returns the height delta to the highest neighbor
function EdgeDetailer:_is_edge(height_map, x, y, threshold)
   local neighbor
   local offset = height_map:get_offset(x, y)
   local value = height_map[offset]
   local width = height_map.width
   local height = height_map.height
   local delta, max_delta

   max_delta = threshold

   if x > 1 then
      neighbor = height_map[offset-1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if x < width then
      neighbor = height_map[offset+1]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y > 1 then
      neighbor = height_map[offset-width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end
   if y < height then
      neighbor = height_map[offset+width]
      delta = neighbor - value
      if delta > max_delta then max_delta = delta end
   end

   if max_delta > threshold then
      return max_delta
   else
      return false
   end
end

-----

-- makes lots of assumptions about how grasslands are quantized
-- ok since this will change anyway if grasslands are quantized differently
function EdgeDetailer:add_grassland_details(height_map)
   local edge_threshold = 2
   local i, j

   for j=1, height_map.width do
      for i=1, height_map.height do
         if self:_is_grassland_edge(height_map, i, j, edge_threshold) then
            local offset = height_map:get_offset(i, j)
            height_map[offset] = height_map[offset] + 1
         end
      end
   end
end

function EdgeDetailer:_is_grassland_edge(height_map, x, y, threshold)
   local offset = height_map:get_offset(x, y)
   local value = height_map[offset]
   local width = height_map.width
   local height = height_map.height
   local neighbor

   if value >= self.terrain_info[TerrainType.Grassland].max_height then
      return false
   end

   if x > 1 then
      neighbor = height_map[offset-1]
      if neighbor - value == threshold then return true end
   end
   if x < width then
      neighbor = height_map[offset+1]
      if neighbor - value == threshold then return true end
   end
   if y > 1 then
      neighbor = height_map[offset-width]
      if neighbor - value == threshold then return true end
   end
   if y < height then
      neighbor = height_map[offset+width]
      if neighbor - value == threshold then return true end
   end

   return false
end

return EdgeDetailer
