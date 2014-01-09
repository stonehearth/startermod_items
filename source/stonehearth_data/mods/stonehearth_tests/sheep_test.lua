local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   self:place_citizen(6, 6) 
   self:place_citizen(6, 8) 
   local sheep = self:place_item('stonehearth:sheep', -3, -6)
   local sheep = self:place_item('stonehearth:sheep', 0, -6)
   local sheep = self:place_item('stonehearth:sheep', 3, -6)
   local sheep = self:place_item('stonehearth:sheep', 6, -6)   
end

return RabbitTest

