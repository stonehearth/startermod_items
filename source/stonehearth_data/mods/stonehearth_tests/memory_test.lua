local MicroWorld = require 'lib.micro_world'
local Memorytest = class(MicroWorld)

function Memorytest:__init()
   self[MicroWorld]:__init()
   self[MicroWorld]._size = 100
   self:create_world()

   for j=1, 30 do
      for i=1, 30 do
         self:place_tree(i, j)
      end
   end
end

return Memorytest

