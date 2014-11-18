local MicroWorld = require 'lib.micro_world'
local CollisionTest = class(MicroWorld)

function CollisionTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth:resources:wood:oak_log', 5, -5, 4, 4);
   self:place_item_cluster('stonehearth:berry_basket', -5, -5, 4, 4);

   local worker = self:place_citizen(13, 13)
   local player_id = worker:get_component('unit_info'):get_player_id()   
---[[
   self:place_item('stonehearth:furniture:picket_fence', 0, 3)
   self:place_item('stonehearth:furniture:picket_fence', 2, 3)
   self:place_item('stonehearth:furniture:picket_fence', 4, 3)
   self:place_item('stonehearth:furniture:picket_fence', 6, 3)
   self:place_item('stonehearth:furniture:picket_fence', 8, 3)
   self:place_item('stonehearth:furniture:picket_fence', 10, 3)
--]]

--[[
   self:place_tree(0, 3)
   self:place_tree(1, 3)
   self:place_tree(2, 3)
   self:place_tree(3, 3)
   self:place_tree(4, 3)
   self:place_tree(5, 3)
   self:place_tree(6, 3)
   self:place_tree(7, 3)

]]

   self:place_stockpile_cmd(player_id, 10, 10, 2, 2)
   
end

return CollisionTest

