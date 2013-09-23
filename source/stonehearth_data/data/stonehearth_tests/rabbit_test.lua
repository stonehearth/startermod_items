local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   local rabbit = self:place_rabbit(0, 0)
   --local bench = self:place_item('stonehearth_carpenter_class', 'carpenter_workbench', -6, 6)
   
end

function RabbitTest:place_rabbit(x, z)
   local rabbit_entity = radiant.entities.create_entity('entity(stonehearth, rabbit)')
   local location = Point3(x, 0, z)

   radiant.terrain.place_entity(rabbit_entity, location)
end

return RabbitTest

