local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local FirePitTest = class(MicroWorld)

function FirePitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(0, -12)
   local worker = self:place_citizen(12, 12)
   local faction = worker:get_component('unit_info'):get_faction()
   local firepit = self:place_item('stonehearth_items.fire_pit', 0, 0)
end

return FirePitTest

