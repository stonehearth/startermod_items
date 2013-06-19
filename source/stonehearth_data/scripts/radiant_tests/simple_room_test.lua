local gm = require 'radiant.core.gm'
local MicroWorld = require 'radiant_tests.micro_world'

local SimpleRoom = class(MicroWorld)
gm:register_scenario('radiant.tests.simple_room', SimpleRoom)

SimpleRoom['radiant.md.create'] = function(self, bounds)
   self:create_world()
   self:place_citizen(12, 12)
   self:place_item_cluster('module://stonehearth/resources/oak_tree/oak_log', 4, 8)
   self:place_stockpile_cmd(12, -12)
   
   local house = self:create_room_cmd(-10, -10, 4, 4)
   
   self:at(100, function() self:start_project_cmd(house) end)
end
