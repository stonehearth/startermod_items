local MicroWorld = require 'lib.micro_world'
local ShopTest = class(MicroWorld)
local Point3 = _radiant.csg.Point3

function ShopTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_juniper_tree', -25, -5)
   self:place_item('stonehearth:medium_juniper_tree', -5, -5)
   self:place_item('stonehearth:small_juniper_tree',  15, -5)

   self:place_item_cluster('stonehearth:loot:gold', -10, 15, 3, 3)
   self:place_item_cluster('stonehearth:pumpkin_harvest', -10, 11, 3, 3)
   
   --local worker = self:place_citizen(12, 12)
   --self:place_citizen(14, 14)
   

   --Place a banner
   --local player_id = worker:get_component('unit_info'):get_player_id()
   local player_id = 'player_1'
   local town = stonehearth.town:get_town(player_id)
   local location = Point3(7, 0, 7)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location, { force_iconic = false })
   town:set_banner(banner_entity)

   self:at(10,  function()
         self:place_stockpile_cmd(player_id, -8, -8, 0, 0)
         stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:caravan_shop')
      end)
end

return ShopTest

