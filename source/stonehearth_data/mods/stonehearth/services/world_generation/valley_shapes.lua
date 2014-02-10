local Array2D = require 'services.world_generation.array_2D'

local ValleyShapes = class()

function ValleyShapes:__init(rng)
   self._rng = rng

   self._shapes = {
      { 
         0, 1, 0,
         1, 1, 0,
         0, 0, 0
      },
      { 
         1, 1, 0,
         1, 1, 0,
         0, 0, 0
      },
       {
         0, 1, 0,
         1, 1, 0,
         0, 1, 0
      },
      {
         0, 1, 0,
         0, 1, 0,
         1, 1, 0
      },
      {
         1, 1, 0,
         0, 1, 0,
         0, 1, 1
      },
      {
         0, 1, 0,
         1, 1, 0,
         1, 1, 0
      },
      {
         1, 1, 0,
         1, 1, 0,
         1, 1, 0
      },
      {
         1, 1, 0,
         1, 1, 1,
         0, 1, 1
      },
      {
         0, 1, 1,
         1, 1, 1,
         1, 1, 1
      },
      {
         0, 1, 1,
         0, 1, 1,
         1, 1, 1
      }
   }

   self._num_shapes = #self._shapes
   self.shape_width = 3
   self.shape_height = 3

   self._shape_buffer = Array2D(self.shape_width, self.shape_height)
   local foo = 1
end

function ValleyShapes:get_random_shape()
   local index, shape
   local block = Array2D(self.shape_width, self.shape_height)

   index = self._rng:get_int(1, self._num_shapes)
   shape = self._shapes[index]
   block:load(shape)
   self:_randomize_orientation(block)

   return block
end

function ValleyShapes:_randomize_orientation(block)
   local rng = self._rng

   -- total of 8 possible orientations
   if rng:get_int(0, 1) == 1 then
      self:_transform_block(block, ValleyShapes._rotate_90)
   end

   if rng:get_int(0, 1) == 1 then
      self:_transform_block(block, ValleyShapes._reflect_x)
   end

   if rng:get_int(0, 1) == 1 then
      self:_transform_block(block, ValleyShapes._reflect_y)
   end
end

function ValleyShapes:_transform_block(block, fn)
   local temp = self._shape_buffer
   -- +1 for base 1 arrays
   local center_x = (block.width+1) * 0.5
   local center_y = (block.height+1) * 0.5
   local x, y, value

   for j=1, block.height do
      for i=1, block.width do
         x, y = ValleyShapes._transform_around(i, j, center_x, center_y, fn)
         value = block:get(x, y)
         temp:set(i, j, value)
      end
   end

   block:load(temp)
end

-- expose the C++ matrix library if this gets any more complex
-- perform a transform on (x, y) using (xc, yc) as the origin
function ValleyShapes._transform_around(x, y, xc, yc, fn)
   x, y = ValleyShapes._translate(x, y, -xc, -yc)
   x, y = fn(x, y)
   x, y = ValleyShapes._translate(x, y, xc, yc)
   return x, y
end

function ValleyShapes._translate(x, y, dx, dy)
   return x+dx, y+dy
end

function ValleyShapes._rotate_90(x, y)
   return -y, x
end

function ValleyShapes._reflect_x(x, y)
   return -x, y
end

function ValleyShapes._reflect_y(x, y)
   return x, -y
end

return ValleyShapes
