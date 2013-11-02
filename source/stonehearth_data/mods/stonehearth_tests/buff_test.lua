local MicroWorld = require 'lib.micro_world'
local BuffTest = class(MicroWorld)

function BuffTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_tree(-9, -9)
   self:place_citizen(5, -5)
   local worker = self:place_citizen(-5, -5)
   local faction = worker:get_component('unit_info'):get_faction()
   radiant.entities.add_buff(worker, 'stonehearth:buffs:groggy')
   
   self:place_item('stonehearth:comfy_bed', 0, 0)

   self:at(10,  function()
         self:place_stockpile_cmd(faction, 12, 12, 4, 4)
      end)
end

return BuffTest

