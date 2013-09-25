local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   self:place('stonehearth.rabbit', 6, 6)
   self:place('stonehearth.wolf', 0, 0)

   local worker = self:place_citizen(-3, -3)

   --local bench = self:place_item('stonehearth_carpenter_class.carpenter_workbench', -6, 6)
   
end

function RabbitTest:place(entityDesc, x, z)
   local rabbit_entity = radiant.entities.create_entity(entityDesc)
   local location = Point3(x, 0, z)

   radiant.terrain.place_entity(rabbit_entity, location)

   local leash = rabbit_entity:get_component('stonehearth:leash')
   leash:set_location(location)

   return rabbit_entity
end

return RabbitTest

