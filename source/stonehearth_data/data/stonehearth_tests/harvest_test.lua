native:log('requiring microworld')

local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   --local worker = self:place_citizen(-8, -8)
   local worker = self:place_citizen(12, 12)
   local faction = worker:get_component('unit_info'):get_faction()
   
   self:at(10,  function()
         --self:place_stockpile_cmd(faction, 4, 12)
      end)

   self:at(100, function()
         tree:get_component('abilities'):do_ability('chop', faction)
      end)
end

return HarvestTest

