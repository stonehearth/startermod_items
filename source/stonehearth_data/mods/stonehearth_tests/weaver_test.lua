local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local WeaverTest = class(MicroWorld)

function WeaverTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_citizen(-12, 4)
   local weaver = self:place_citizen(-12, 7, 'weaver')
   local player_id = radiant.entities.get_player_id(weaver)

   -- put some items in the world
   self:place_item_cluster('stonehearth:oak_log', -10, 0, 10, 10)
   self:place_item_cluster('stonehearth:silkweed_bundle', 10, 3, 5, 5)
   self:place_item('stonehearth:worker:outfit:2',  -5, 13)

end

return WeaverTest