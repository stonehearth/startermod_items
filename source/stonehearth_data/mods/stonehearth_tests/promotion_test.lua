local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local PromoteTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function PromoteTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the saw 
   self:place_item('stonehearth:carpenter:saw', 4, 0)

   self:place_citizen(-1,2)
   local worker = self:place_citizen(12, 12)

   self:place_item_cluster('stonehearth:oak_log', 6, 6, 2, 2);
   
   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)
end

return PromoteTest
