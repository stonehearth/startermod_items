local MicroWorld = require 'lib.micro_world'
local BuffTest = class(MicroWorld)

function BuffTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_tree(-9, -9)
   local citizen_1 = self:place_citizen(5, -5)
   local citizen_2 = self:place_citizen(-5, -5)
   local citizen_3 = self:place_citizen(0, 0)
   local faction = citizen_2:get_component('unit_info'):get_faction()
   radiant.entities.add_buff(citizen_2, 'stonehearth:buffs:groggy')

   radiant.entities.add_buff(citizen_1, 'stonehearth:buffs:starving')
   radiant.entities.add_buff(citizen_1, 'stonehearth:buffs:groggy')

   local bush = self:place_item('stonehearth:berry_bush', 4, -4)
   radiant.entities.add_buff(citizen_2, 'stonehearth:buffs:starving')

   self:place_item('stonehearth:comfy_bed', 0, 0)

   self:at(10,  function()
         self:place_stockpile_cmd(faction, 12, 12, 4, 4)
      end)
end

return BuffTest

