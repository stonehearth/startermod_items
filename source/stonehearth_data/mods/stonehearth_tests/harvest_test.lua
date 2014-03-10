local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth:terrain:tall_grass', -8, 2, 6, 4)
   self:place_item_cluster('stonehearth:terrain:tall_grass', -8, 8, 2, 2)
   self:place_item('stonehearth:medium_oak_tree', -9, -9)
   self:place_item('stonehearth:medium_oak_tree', 12, -2)
   self:place_item('stonehearth:small_boulder', 1, 1)
   self:place_item('stonehearth:medium_boulder', 4, 4)
   self:place_item('stonehearth:large_boulder', 8, 8)
   self:place_citizen(12, 12, 'trapper')
   self:place_citizen(10, 10)

   
   self:at(10,  function()
         --self:place_stockpile_cmd(faction, 12, 12, 4, 4)
      end)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', faction)
      end)
end

return HarvestTest

