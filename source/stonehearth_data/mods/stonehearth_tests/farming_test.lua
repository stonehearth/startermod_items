local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local FarmingTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function FarmingTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --place the hoes and board
   self:place_item('stonehearth:farmer:hoe_proxy', 4, 1)
   self:place_item('stonehearth:farmer:hoe_proxy', 4, 2)

   local farmer = self:place_citizen(-1,2, 'farmer')
   self:place_citizen(-1,5)

   
   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)

   local session = {
      player_id = radiant.entities.get_player_id(farmer),
      kingdom = radiant.entities.get_kingdom(farmer)
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:tester_crop')
end

return FarmingTest

