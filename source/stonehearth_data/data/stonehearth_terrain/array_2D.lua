local Array2D = class()

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

return Array2D
