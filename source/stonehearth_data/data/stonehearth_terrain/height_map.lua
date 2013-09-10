local MathFns = radiant.mods.require('/stonehearth_terrain/math/math_fns.lua')

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

function HeightMap:clone()
   local dst = new HeightMap(self.width, self.height)
   local size = self.width * self.height
   local i

   for i=1, size, 1 do
      dst[i] = self[i]
   end

   return dst
end

function HeightMap:clear(value)
   self:process_map(function () return value end)
end

function HeightMap:set_block(x, y, block_width, block_height, value)
   self:process_map_block(x, y, block_width, block_height,
      function () return value end
   )
end

function HeightMap:copy_block(dst, src, dstx, dsty, srcx, srcy, block_width, block_height)
   local i, j
   local dst_offset = dst:get_offset(dstx, dsty)-1
   local src_offset = src:get_offset(srcx, srcy)-1

   for j=1, block_height, 1 do
      for i=1, block_width, 1 do
         dst[dst_offset+i] = src[src_offset+i]
      end
      dst_offset = dst_offset + dst.width
      src_offset = src_offset + src.width
   end
end

function HeightMap:process_map(func)
   local i
   local size = self.width * self.height

   for i=1, size, 1 do
      self[i] = func(self[i])
   end
end

function HeightMap:process_map_block(x, y, block_width, block_height, func)
   local i, j, index
   local offset = self:get_offset(x, y)-1

   for j=1, block_height, 1 do
      for i=1, block_width, 1 do
         index = offset+i
         self[index] = func(self[index])
      end
      offset = offset + self.width
   end
end

function HeightMap:print()
   local i, j, str

   for j=1, self.height, 1 do
      str = ""
      for i=1, self.width, 1 do
         str = str .. " " .. string.format("%6.1f" , self:get(i, j))
      end
      radiant.log.info(str)
   end
end

return HeightMap
