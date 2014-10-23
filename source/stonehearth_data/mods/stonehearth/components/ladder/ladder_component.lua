local Point3 = _radiant.csg.Point3
local TraceCategories = _radiant.dm.TraceCategories

local Ladder = class()

function Ladder:initialize(entity, json)
   self._sv.desired_height = 0
   self.__saved_variables:mark_changed()
end

function Ladder:trace(reason)
   return self.__saved_variables:trace(reason)
end

function Ladder:get_normal()
   return self._sv.normal
end

function Ladder:set_normal(normal)
   assert(normal.y == 0, string.format('invalid normal %s in :set_normal()', tostring(normal)))
   if normal ~= self._sv.normal then
      self._sv.normal = normal
      self.__saved_variables:mark_changed()
   end
   return self
end

function Ladder:get_desired_height()
   return self._sv.desired_height
end

function Ladder:set_desired_height(height)
   if height ~= self._sv.desired_height then
      self._sv.desired_height = height
      self.__saved_variables:mark_changed()
   end
   return self
end

return Ladder

