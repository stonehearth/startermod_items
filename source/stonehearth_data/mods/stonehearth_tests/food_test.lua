local MicroWorld = require 'lib.micro_world'
local FoodTest = class(MicroWorld)

function FoodTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local bush = self:place_item('stonehearth:berry_bush', 4, 4)
   local bush = self:place_item('stonehearth:berry_bush', -1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -4, 4)
   local bush = self:place_item('stonehearth:berry_bush', 4, -4)

   self:place_item('stonehearth:worker:outfit:2', 10, 10)


   self:place_item_cluster('stonehearth:resources:fiber:wool_bundle', 8, 8, 4, 4)

   self:place_item('stonehearth:furniture:arch_backed_chair', 6, 6)

   local tree = self:place_tree(-12, -12)

   --self:place_citizen(10, 10)
   local worker = self:place_citizen(-5, -5)

   local player_id = worker:get_component('unit_info'):get_player_id()
   
   --self:place_item('stonehearth:food:berries:berry_basket', 0, 0)
   --self:place_item('stonehearth:food:corn:corn_basket', 1, 1)
   radiant.set_realtime_timer(5, function()
         worker:get_component('stonehearth:attributes')
                  :set_attribute('calories', stonehearth.constants.food.MALNOURISHED)
      end)   
end

return FoodTest

