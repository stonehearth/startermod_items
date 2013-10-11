local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local FirePitTest = class(MicroWorld)

function FirePitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(0, -12)
   local worker = self:place_citizen(12, 12)
   local worker2 = self:place_citizen(-11, -11)

   self:place_item('stonehearth:fire_pit', 1, 1)
end

return FirePitTest

