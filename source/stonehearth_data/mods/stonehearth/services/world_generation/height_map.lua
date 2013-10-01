local MathFns = require 'services.world_generation.math.math_fns'

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

function HeightMap:is_boundary(x, y)
   if x == 1 or y == 1 then return true end
   if x == self.width or y == self.height then return true end
   return false
end

function HeightMap:clone()
   local dst = new HeightMap(self.width, self.height)
   local size = self.width * self.height
   local i

   for i=1, size do
      dst[i] = self[i]
   end

   return dst
end

function HeightMap:clear(value)
   local function fn() return value end
   self:process_map(fn)
end

function HeightMap:set_block(x, y, block_width, block_height, value)
   local function fn() return value end
   self:process_map_block(x, y, block_width, block_height, fn)
end

function HeightMap:copy_block(dst, src, dstx, dsty, srcx, srcy, block_width, block_height)
   local i, j
   local dst_offset = dst:get_offset(dstx, dsty)-1
   local src_offset = src:get_offset(srcx, srcy)-1

   for j=1, block_height do
      for i=1, block_width do
         dst[dst_offset+i] = src[src_offset+i]
      end
      dst_offset = dst_offset + dst.width
      src_offset = src_offset + src.width
   end
end

function HeightMap:process_map(func)
   local i
   local size = self.width * self.height

   for i=1, size do
      self[i] = func(self[i])
   end
end

function HeightMap:process_map_block(x, y, block_width, block_height, func)
   local i, j, index
   local offset = self:get_offset(x, y)-1

   for j=1, block_height do
      for i=1, block_width do
         index = offset+i
         self[index] = func(self[index])
      end
      offset = offset + self.width
   end
end

function HeightMap:print(format_string)
   if format_string == nil then format_string = '%6.1f' end
   local i, j, str

   for j=1, self.height do
      str = ''
      for i=1, self.width do
         str = str .. ' ' .. string.format(format_string, self:get(i, j))
      end
      radiant.log.info(str)
   end
end

return HeightMap
