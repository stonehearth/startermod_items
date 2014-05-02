local MicroWorld = require 'lib.micro_world'
local StockpileTest = class(MicroWorld)

function StockpileTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth:oak_log', -15, -15, 5, 5);

   --self:place_citizen(15, 15)
   local worker = self:place_citizen(12, 12)
   for i = 1, 10 do
      self:place_citizen(12, 12)
   end
   local player_id = worker:get_component('unit_info'):get_player_id()   
   self:place_stockpile_cmd(player_id, -10, -10, 5, 5)
   if true then return end
   
   --local log = radiant.entities.create_entity('stonehearth:oak_log')
   --worker:get_component('stonehearth:carry_block'):set_carrying(log)
   
   --local worker = self:place_citizen(2, 4)
   self:place_citizen(12, 12)
   self:place_citizen(13, 12) 
   self:place_citizen(14, 12)
   --self:place_citizen(15, 12)
end

return StockpileTest

