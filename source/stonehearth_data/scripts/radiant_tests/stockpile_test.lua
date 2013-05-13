local MicroWorld = require 'radiant_tests.micro_world'

local StockpileTest = class(MicroWorld)
local gm = require 'radiant.core.gm'

StockpileTest['radiant.md.create'] = function(self, bounds)
   self:create_world()
   self:place_item_cluster('module://stonehearth/resources/oak_tree/oak_log', -14, -14, 6, 6);
   self:place_citizen(11, 12)
   self:place_citizen(12, 12)
   self:place_citizen(13, 12)
   self:place_citizen(14, 12)
   self:place_citizen(15, 12)
   self:place_stockpile_cmd(4,  12, 2, 2)
   self:place_stockpile_cmd(-4, 12, 2, 2)
end

gm:register_scenario('radiant.tests.stockpile', StockpileTest)
