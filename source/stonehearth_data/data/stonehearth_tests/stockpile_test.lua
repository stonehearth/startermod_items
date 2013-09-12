local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local StockpileTest = class(MicroWorld)

function StockpileTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth_trees', 'oak_log', -15, -15, 4, 4);
   --self:place_citizen(15, 15)
   local worker = self:place_citizen(13, 13)
   local faction = worker:get_component('unit_info'):get_faction()   
   --self:place_citizen(12, 12)
   --self:place_citizen(13, 12)
   --self:place_citizen(14, 12)
   --self:place_citizen(15, 12)
   self:place_stockpile_cmd(faction, 10, 10, 2, 2)
   

end

return StockpileTest

