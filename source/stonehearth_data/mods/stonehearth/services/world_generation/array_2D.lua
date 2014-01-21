local Array2D = class()
local log = radiant.log.create_logger('world_generation')

function Array2D:__init(width, height)
   self.width = width
   self.height = height
end

function Array2D:get(x, y)
   return self[self:get_offset(x,y)]
end

function Array2D:set(x, y, value)
   self[self:get_offset(x,y)] = value
end

function Array2D:get_offset(x, y)
   return (y-1)*self.width + x
end

function Array2D:is_boundary(x, y)
   if x == 1 or y == 1 then return true end
   if x == self.width or y == self.height then return true end
   return false
end

function Array2D:in_bounds(x, y)
   if x < 1 or y < 1 or
      x > self.width or y > self.width then
      return false
   end
   return true
end

function Array2D:bound(x, y)
   if x < 1 then x = 1
   elseif x > self.width then x = self.width
   end

   if y < 1 then y = 1
   elseif y > self.height then y = self.height
   end

   return x, y
end

function Array2D:clone()
   local dst = Array2D(self.width, self.height)
   local size = self.width * self.height
   local i

   for i=1, size do
      dst[i] = self[i]
   end

   return dst
end

function Array2D:clear(value)
   local function fn() return value end
   self:process_map(fn)
end

function Array2D:fill(fn)
   local i, j, offset

   offset = 1
   for j=1, self.height do
      for i=1, self.width do
         self[offset] = fn(i, j)
         offset = offset + 1
      end
   end
end

function Array2D:set_block(x, y, block_width, block_height, value)
   local function fn() return value end
   self:process_block(x, y, block_width, block_height, fn)
end

function Array2D:process_map(fn)
   local i
   local size = self.width * self.height

   for i=1, size do
      self[i] = fn(self[i])
   end
end

function Array2D:process_block(x, y, block_width, block_height, fn)
   local i, j, index
   local offset = self:get_offset(x, y)-1

   for j=1, block_height do
      for i=1, block_width do
         index = offset+i
         self[index] = fn(self[index])
      end
      offset = offset + self.width
   end
end

-- terminates early if fn(x) returns false on an element
-- returns true if fn(x) returns true for all elements, false otherwise
function Array2D:visit_block(x, y, block_width, block_height, fn)
   local i, j, index, continue
   local offset = self:get_offset(x, y)-1

   for j=1, block_height do
      for i=1, block_width do
         index = offset+i
         continue = fn(self[index])
         if not continue then
            return false
         end
      end
      offset = offset + self.width
   end
   return true
end

function Array2D:print(format_string)
   if format_string == nil then format_string = '%6.1f' end
   local i, j, str

   for j=1, self.height do
      str = ''
      for i=1, self.width do
         str = str .. ' ' .. string.format(format_string, self:get(i, j))
      end
      log:debug(str)
   end
end

----- Static functions -----

function Array2D.copy_block(dst, src, dstx, dsty, srcx, srcy, block_width, block_height)
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

function Array2D.copy_vector(dst, src, dst_start, dst_inc, src_start, src_inc, length)
   local i
   local x = src_start
   local y = dst_start

   for i=1, length do
      dst[y] = src[x]
      x = x + src_inc
      y = y + dst_inc
   end
end

function Array2D.get_row_vector(dst, src, row_num, row_width, length)
   Array2D.copy_vector(dst, src, 1, 1, (row_num-1)*row_width+1, 1, length)
end

function Array2D.set_row_vector(dst, src, row_num, row_width, length)
   Array2D.copy_vector(dst, src, (row_num-1)*row_width+1, 1, 1, 1, length)
end

function Array2D.get_column_vector(dst, src, col_num, row_width, length)
   Array2D.copy_vector(dst, src, 1, 1, col_num, row_width, length)
end

function Array2D.set_column_vector(dst, src, col_num, row_width, length)
   Array2D.copy_vector(dst, src, col_num, row_width, 1, 1, length)
end

return Array2D
