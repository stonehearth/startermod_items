local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_tree(-9, -9)
   self:place_tree(12, -2)
   self:place_citizen(12, 12)
   if true then return end
   self:place_citizen(8, 10)
   self:place_citizen(0, 0)
   local worker = self:place_citizen(-5, -5)
   local faction = worker:get_component('unit_info'):get_faction()

   self:at(10,  function()
         self:place_stockpile_cmd(faction, 12, 12, 4, 4)
      end)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', faction)
      end)
end

return HarvestTest

