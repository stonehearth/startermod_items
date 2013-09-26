local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_tree(-12, -12)
  
   self:place_item('stonehearth.wolf', -12, 12)
   self:place_citizen(0, 0)

   --local bench = self:place_item('stonehearth_carpenter_class.carpenter_workbench', -6, 6)
   
end

return RabbitTest

