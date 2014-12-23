local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local ShepherdTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function ShepherdTest:__init()
   self[MicroWorld]:__init(256)
   self:create_world()

   --place the hoes and board
   self:place_item('stonehearth:shepherd:talisman', 4, 2)

   local bush = self:place_item('stonehearth:berry_bush', 4, 4)
   local bush = self:place_item('stonehearth:berry_bush', -1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -4, 4)
   local bush = self:place_item('stonehearth:berry_bush', 4, -4)

   self:place_citizen(-1,5)
   self:place_citizen(-1,2, 'shepherd')
   self:place_citizen(-1,7, 'trapper')
   
   self:place_item('stonehearth:sheep', -3, -6)

   self:place_item_cluster('stonehearth:furniture:picket_fence', -13, 0, 6, 6)
   self:place_item('stonehearth:furniture:picket_fence_gate', 1, 3)
   self:place_item('stonehearth:trapper:talisman', 10, 10)
end

return ShepherdTest

