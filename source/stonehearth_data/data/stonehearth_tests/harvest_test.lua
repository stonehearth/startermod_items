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
         self:place_stockpile_cmd(faction, 12, 12, 4, 4)
      end)

   self:at(100, function()
         --tree:get_component('radiant:commands'):do_command('chop', faction)
         --/server/objects/stonehearth_census/worker_tracker
      end)
end

return HarvestTest

