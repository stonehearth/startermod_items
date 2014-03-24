local MathFns = require 'services.server.world_generation.math.math_fns'

local Array2D = class()

function Array2D:__init(width, height)
   self.width = width
   self.height = height
end

function Array2D:get(x, y)
   --assert(self:in_bounds(x, y))
   return self[self:get_offset(x,y)]
end

function Array2D:set(x, y, value)
   --assert(self:in_bounds(x, y))
   self[self:get_offset(x,y)] = value
end

function Array2D:get_offset(x, y)
   --assert(self:in_bounds(x, y))
   return (y-1)*self.width + x
end

function Array2D:get_dimensions()
   return self.width, self.height
end

function Array2D:is_boundary(x, y)
   if x == 1 or y == 1 then
      return true
   end

   if x == self.width or y == self.height then
      return true
   end

   return false
end

function Array2D:in_bounds(x, y)
   if x < 1 or y < 1 or
      x > self.width or y > self.height then
      return false
   end
   return true
end

function Array2D:bound(x, y)
   if x < 1 then
      x = 1
   elseif x > self.width then
      x = self.width
   end

   if y < 1 then
      y = 1
   elseif y > self.height then
      y = self.height
   end

   return x, y
end

-- return 0 width/height for non-intersecting block
function Array2D:bound_block(x, y, width, height)
   -- x1, y1 is inclusve
   local x1 = MathFns.bound(x, 1, self.width+1)
   local y1 = MathFns.bound(y, 1, self.height+1)

   -- x2, y2 is exclusive
   local x2 = MathFns.bound(x+width, 1, self.width+1)
   local y2 = MathFns.bound(y+height, 1, self.height+1)

   local new_width = x2 - x1
   local new_height = y2 - y1

   return x1, y1, new_width, new_height
end

function Array2D:block_in_bounds(x, y, width, height)
   if not self:in_bounds(x, y) then
      return false
   end

   local x2 = x + width-1
   local y2 = y + height-1

   local result = self:in_bounds(x2, y2)
   return result
end

-- does not test diagonals
function Array2D:adjacent_to(value, x, y)
   local offset = self:get_offset(x, y)

   if x > 1 then
      if value == self[offset-1] then
         return true
      end
   end

   if y > 1 then 
      if value == self[offset-width] then
         return true
      end
   end

   if x < self.width then
      if value == self[offset+1] then
         return true
      end
   end

   if y < self.height then
      if value == self[offset+width] then
         return true
      end
   end

   return false
end

function Array2D:clone()
   local dst = Array2D(self.width, self.height)
   local size = self.width * self.height

   for i=1, size do
      dst[i] = self[i]
   end

   return dst
end

function Array2D:clone_to_nested_arrays()
   local dst = {}
   local offset, row

   for j=1, self.height do
      row = {}
      offset = self:get_offset(1, j) - 1
      for i=1, self.width do
         row[i] = self[offset+i]
      end

      dst[j] = row
   end

   return dst
end

function Array2D:fill_ij(fn)
   local offset = 1

   for j=1, self.height do
      for i=1, self.width do
         self[offset] = fn(i, j)
         offset = offset + 1
      end
   end
end

-- don't rename to set(value), will overwrite set(x, y, value)
function Array2D:clear(value)
   local size = self.width * self.height

   for i=1, size do
      self[i] = value
   end
end

function Array2D:set_block(x, y, block_width, block_height, value)
   local function fn() return value end
   self:process_block(x, y, block_width, block_height, fn)
end

function Array2D:process(fn)
   local size = self.width * self.height

   for i=1, size do
      self[i] = fn(self[i])
   end
end

function Array2D:process_block(x, y, block_width, block_height, fn)
   local index
   local offset = self:get_offset(x, y)-1

   for j=1, block_height do
      for i=1, block_width do
         index = offset+i
         self[index] = fn(self[index])
      end
      offset = offset + self.width
   end
end

-- returns true if fn(x) returns true for all elements, false otherwise
-- terminates early if fn(x) returns false on an element
function Array2D:visit(fn)
   local size = self.width * self.height
   local continue

   for i=1, size do
      continue = fn(self[i])
      if not continue then
         return false
      end
   end
   return true
end

-- returns true if fn(x) returns true for all elements, false otherwise
-- terminates early if fn(x) returns false on an element
function Array2D:visit_block(x, y, block_width, block_height, fn)
   local index, continue
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

function Array2D:load(array)
   local size = self.width * self.height

   for i=1, size do
      self[i] = array[i]
   end
end

function Array2D:print(logger, format_string)
   if format_string == nil then format_string = '%3.0f' end
   local str

   for j=1, self.height do
      str = ''
      for i=1, self.width do
         str = str .. ' ' .. string.format(format_string, self:get(i, j))
      end
      logger:debug(str)
   end
end

----- Static functions -----

function Array2D.copy_block(dst, src, dstx, dsty, srcx, srcy, block_width, block_height)
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
