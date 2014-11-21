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
   
   self:place_item('stonehearth:sheep', -3, -6)

   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)

end

return ShepherdTest

