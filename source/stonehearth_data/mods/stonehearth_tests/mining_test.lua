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

   local citizen = self:place_citizen(15, 0)
   local citizen = self:place_citizen(15, 15)

   local point = Point3(20, 9, 4)
   local region = Region3(Cube3(point, point + Point3(1, 1, 1)))

   local mining_zone = stonehearth.mining:create_mining_zone(session.player_id, session.faction)
   stonehearth.mining:dig_down(mining_zone, region)

   for i = 1, 2 do
      region:translate(Point3(4, 0, 0))
      stonehearth.mining:dig_out(mining_zone, region)
   end
end

return MiningTest
