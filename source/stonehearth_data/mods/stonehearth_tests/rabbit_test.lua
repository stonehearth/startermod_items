local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   local rabbit = self:place_item('stonehearth:rabbit', 0, 0)
   rabbit:add_component('stonehearth:leash'):set_to_entity_location(rabbit)

   --local bench = self:place_item('stonehearth:carpenter:workbench', -6, 6)
   
end

return RabbitTest

