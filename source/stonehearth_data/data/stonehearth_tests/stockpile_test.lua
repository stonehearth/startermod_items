local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local StockpileTest = class(MicroWorld)

function StockpileTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('mod://stonehearth_trees/entities/oak_tree/oak_log', -15, -15, 2, 2);
   local worker = self:place_citizen(15, 15)
   local faction = worker:get_component('unit_info'):get_faction()   
   --self:place_citizen(12, 12)
   --self:place_citizen(13, 12)
   --self:place_citizen(14, 12)
   --self:place_citizen(15, 12)
   self:place_stockpile_cmd(faction,  -1, -1, 3, 3)
end

return StockpileTest

