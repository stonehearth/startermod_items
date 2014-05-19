local MicroWorld = require 'lib.micro_world'
local FoodTest = class(MicroWorld)

function FoodTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local bush = self:place_item('stonehearth:berry_bush', 4, 4)
   local bush = self:place_item('stonehearth:berry_bush', -1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -4, 4)
   local bush = self:place_item('stonehearth:berry_bush', 4, -4)

   self:place_item_cluster('stonehearth:wool_bundle', 8, 8, 4, 4)

   self:place_item('stonehearth:arch_backed_chair_proxy', 6, 6)

   local tree = self:place_tree(-12, -12)

   --self:place_citizen(10, 10)
   local worker = self:place_citizen(-5, -5)

   local player_id = worker:get_component('unit_info'):get_player_id()
   
   self:place_item('stonehearth:berry_basket', 0, 0)
   self:place_item('stonehearth:corn_basket', 1, 1)
   
   self:at(10000,  function()
         worker:get_component('stonehearth:attributes'):set_attribute('calories', stonehearth.constants.food.MALNOURISHED)
         --self:place_stockpile_cmd(player_id, 8, 8, 4, 4)
      end)

   self:at(100, function()
         --stonehearth.calendar:set_time_unit_test_only({ hour = 23, minute = 58 })

         --bush:get_component('stonehearth:commands'):do_command('chop', player_id)
      end)
end

return FoodTest

