local MicroWorld = require 'lib.micro_world'
local FoodTest = class(MicroWorld)

function FoodTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local bush = self:place_item('stonehearth:berry_bush', 4, 4)
   local bush = self:place_item('stonehearth:berry_bush', -1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -4, 4)
   local bush = self:place_item('stonehearth:berry_bush', 4, -4)

   self:place_item('stonehearth:arch_backed_chair_proxy', 6, 6)

   local tree = self:place_tree(-12, -12)

   self:place_citizen(10, 10)
   local worker = self:place_citizen(-5, -5)
   worker:add_component('stonehearth:attributes'):set_attribute('hunger', 70)

   local faction = worker:get_component('unit_info'):get_faction()
   
   --self:place_item('stonehearth:berry_basket', 0, 0)
   
   self:at(10,  function()
         --self:place_stockpile_cmd(faction, 8, 8, 4, 4)
      end)

   self:at(100, function()
         --bush:get_component('stonehearth:commands'):do_command('chop', faction)
      end)
end

return FoodTest

