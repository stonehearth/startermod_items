local Point3 = _radiant.csg.Point3

local WorkerClass = class()

--[[
   A controller that manages all the relevant data about the worker class
]]

function WorkerClass:initialize(entity)
   self._sv.entity = entity
   self._sv.last_gained_lv = 0
end

function WorkerClass:restore()
end

function WorkerClass:promote()
end

function WorkerClass:demote()
end

return WorkerClass
