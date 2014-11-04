local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local MiningTest = class(MicroWorld)

function MiningTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   local session = self:get_session()

   self:place_item_cluster('stonehearth:oak_log', 10, 0, 4, 4)
   self:place_item_cluster('stonehearth:furniture:wooden_garden_lantern', 10, 5, 2, 2)
   self:place_item('stonehearth:firepit', 10, 8)
   self:place_item('stonehearth:small_boulder', -1, -1)

   local citizen = self:place_citizen(21, 5)
   local citizen = self:place_citizen(15, 2)
   local citizen = self:place_citizen(15, 4)
   local citizen = self:place_citizen(15, 6)
   local citizen = self:place_citizen(15, 8)
   local citizen = self:place_citizen(15, 10)

   local point = Point3(20, 9, 4)
   local region = Region3(Cube3(point, point + Point3(1, 1, 1)))

   stonehearth.mining:dig_down(session.player_id, region)

   for i = 1, 2 do
      region:translate(Point3(4, 0, 0))
      stonehearth.mining:dig_out(session.player_id, region)
   end
end

return MiningTest
