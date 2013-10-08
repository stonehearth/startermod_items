local MicroWorld = require 'lib.micro_world'
local SimpleRoomTest = class(MicroWorld)

function SimpleRoomTest:__init()
   self[MicroWorld]:__init()

   self:create_world()
   local worker = self:place_citizen(12, 12)
   local faction = worker:get_component('unit_info'):get_faction()
   --self:place_item_cluster('stonehearth:oak_log', 4, 8)
   --self:place_stockpile_cmd(faction, 12, -12)
   
   local house = self:create_room(faction, -16, -16, 4, 4)
   self:at(10, function()
         -- self:start_project_cmd(house)
      end)
end

return SimpleRoomTest

