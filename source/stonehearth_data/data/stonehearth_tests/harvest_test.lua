native:log('requiring microworld')

local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   self:place_citizen(12, 12)
   self:at(10,  function()
         self:place_stockpile_cmd(4, 12)
      end)
   self:at(100, function()
         radiant.components.get_component(tree, 'abilities'):do_ability('chop')
      end)
end

return HarvestTest

