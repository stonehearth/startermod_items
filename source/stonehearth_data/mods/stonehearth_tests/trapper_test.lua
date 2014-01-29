local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local TrapperTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function TrapperTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local worker = self:place_citizen(-7, -7)
   local trapper = self:place_citizen(7, 7,'trapper')
   local trapper = self:place_citizen(-7, 7,'trapper')
   local rabbit = self:place_item('stonehearth:rabbit', 0, 0)
   local rabbit = self:place_item('stonehearth:rabbit', -3, -6)
   local rabbit = self:place_item('stonehearth:rabbit', 0, -6)

end

return TrapperTest

