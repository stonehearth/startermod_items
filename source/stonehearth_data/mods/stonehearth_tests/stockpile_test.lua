local MicroWorld = require 'lib.micro_world'
local StockpileTest = class(MicroWorld)

function StockpileTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth:oak_log', -15, -15, 8, 8);
   --self:place_citizen(15, 15)
   local worker = self:place_citizen(0, 1)
   local log = radiant.entities.create_entity('stonehearth:oak_log')
   worker:get_component('stonehearth:carry_block'):set_carrying(log)
   
   --local worker = self:place_citizen(2, 4)
   local faction = worker:get_component('unit_info'):get_faction()   
   --self:place_citizen(12, 12)
   --self:place_citizen(13, 12)
   --self:place_citizen(14, 12)
   --self:place_citizen(15, 12)
   self:place_stockpile_cmd(faction, 10, 10, 2, 2)
end

return StockpileTest

