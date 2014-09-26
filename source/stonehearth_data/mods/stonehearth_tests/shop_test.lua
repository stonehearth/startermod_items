local MicroWorld = require 'lib.micro_world'
local QuestTest = class(MicroWorld)

function QuestTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_juniper_tree', -25, -5)
   self:place_item('stonehearth:medium_juniper_tree', -5, -5)
   self:place_item('stonehearth:small_juniper_tree',  15, -5)

   self:place_item_cluster('stonehearth:loot:gold_chest', -10, 15, 3, 3)

   self:place_citizen(12, 12)
   self:place_citizen(14, 14)
   
   self:at(10,  function()
         stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:caravan_shop')
      end)
end

return QuestTest

