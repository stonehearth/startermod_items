local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   self:place_item('stonehearth:rabbit', 6, 6)

   --local bench = self:place_item('stonehearth:carpenter_workbench', -6, 6)
   
end

return RabbitTest

