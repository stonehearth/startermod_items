local HeightMap = class()

function HeightMap:__init(width, height)
   self.width = width
   self.height = height
end

function HeightMap:get(x, y)
   return self[self:get_offset(x,y)]
end

function HeightMap:set(x, y, value)
   self[self:get_offset(x,y)] = value
end

function HeightMap:get_offset(x, y)
   return (y-1)*self.width + x
end

function HeightMap:scale_map(factor)
   self:process_map(
      function (value) return value*factor end
   )
end

function HeightMap:quantize_map_height(step_size)
   local half_step = step_size/2

   self:process_map(
      function (value)
         local temp = value + half_step
         local remainder = temp % step_size
         return temp - remainder
      end
   )
end

function HeightMap:normalize_map_height(base_height)
   local min, offset
   min = self:find_map_min()
   offset = min-base_height
   self:process_map(
      function (value) return value-offset end
   )
end

function HeightMap:find_map_min()
   local LARGE_NUM = 2000000000
   local min = LARGE_NUM
   self:process_map(
      function (value)
         if value < min then min = value end
         return value
      end
   )
   return min
end

function HeightMap:process_map(func)
   local i
   local size = self.width * self.height

   for i=1, size, 1 do
      self[i] = func(self[i])
   end
end

function HeightMap:set_block(x, y, block_width, block_height, value)
   local i, j
   local offset = self:get_offset(x, y)

   for j=1, block_height, 1 do
      for i=1, block_width, 1 do
         self[offset+i-1] = value
      end
      offset = offset + self.width
   end
end

function HeightMap:copy_map_to(dst)
   local i
   local size = self.width * self.height

   dst.width = self.width
   dst.height = self.height

   for i=1, size, 1 do
      dst[i] = self[i]
   end
end

-- move this to another class?
-- returns true if any non-diagonal neighbor has a larger value
function HeightMap:has_higher_neighbor(x, y)
   local neighbor
   local offset = self:get_offset(x, y)
   local value = self[offset]

   if x > 1 then
      neighbor = self[offset-1]
      if neighbor > value then return true end
   end
   if x < self.width then
      neighbor = self[offset+1]
      if neighbor > value then return true end
   end
   if y > 1 then
      neighbor = self[offset-self.width]
      if neighbor > value then return true end
   end
   if y < self.height then
      neighbor = self[offset+self.width]
      if neighbor > value then return true end
   end
   return false
end

return HeightMap
