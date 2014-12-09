local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local ShepherdTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function ShepherdTest:__init()
   self[MicroWorld]:__init(128)
   self:create_world()

   --place the hoes and board
   self:place_item('stonehearth:shepherd:crook_talisman', 4, 2)

   self:place_citizen(-1,5)
   self:place_citizen(-1,2, 'shepherd')
   self:place_citizen(-1,7, 'trapper')
   
   self:place_item('stonehearth:sheep', -3, -6)

   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)

   self:place_item_cluster('stonehearth:furniture:picket_fence', -10, 0, 10, 10)
   self:place_item('stonehearth:furniture:picket_fence_gate', 1, 3)
   self:place_item('stonehearth:food:berries:berry_basket', 10, 10)
end

return ShepherdTest

