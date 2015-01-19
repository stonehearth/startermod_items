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
   self:place_item_cluster('stonehearth:resources:rabbit_pelt', -10, -10, 2, 2)
   self:place_item_cluster('stonehearth:resources:wood:oak_log', -10, 0, 10, 10)
   self:place_item_cluster('stonehearth:resources:fiber:silkweed_bundle', 8, 0, 5, 5)
   self:place_item_cluster('stonehearth:refined:cloth_bolt', 8, 5, 3, 3)
   self:place_item_cluster('stonehearth:refined:thread', 12, 5, 3, 3)
   self:place_item_cluster('stonehearth:resources:fiber:wool_bundle', 8, 8, 3, 3)
   self:place_item_cluster('stonehearth:resources:rabbit_pelt', 12, 8, 3, 3)

   self:place_item('stonehearth:weaver:spinning_wheel', 5, 1, player_id, { force_iconic = false })
   self:place_item('stonehearth:weaver:loom', 5, 5, player_id, { force_iconic = false })

   self:place_item_cluster('stonehearth:worker:outfit:winter', -12, 12, 3, 3)
end

return WeaverTest
