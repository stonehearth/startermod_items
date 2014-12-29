local Point3 = _radiant.csg.Point3
local PeonClass = class()

--[[
   A controller that manages all the relevant data about the worker class
   Peons don't level up, so this is all that's needed.
]]

function PeonClass:initialize(entity)
   self._sv.no_levels = true
end

return PeonClass
