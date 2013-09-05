native:log('requiring microworld')

local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local NewWorld = class(MicroWorld)

function NewWorld:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   --local worker = self:place_citizen(-8, -8)
   
   local worker = self:place_citizen(-3, -3)
   self:place_citizen( 0, -3)
   self:place_citizen( 3, -3)
   self:place_citizen(-3,  3)
   self:place_citizen( 0,  3)
   self:place_citizen( 3,  3)
   self:place_citizen(-3,  0)
   self:place_citizen( 3,  0)

   local faction = worker:get_component('unit_info'):get_faction()
   
   self:place_stockpile_cmd(faction, 8, -2, 4, 4)
   
end

return NewWorld

