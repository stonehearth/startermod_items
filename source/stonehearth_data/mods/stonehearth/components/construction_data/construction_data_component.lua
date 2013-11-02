local ConstructionDataComponent = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- xxx: it would be nice if we could update the datastore once at an appropriate
-- time in the gameloop instead of everytime the normal or tangent changes.
function ConstructionDataComponent:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self._data = {}  
   self._data_binding:update(self._data)
end

function ConstructionDataComponent:extend(data)
   self._data = data
   self._data_binding:update(self._data)
end

function ConstructionDataComponent:set_normal(normal)
   self._data.normal = normal
   self._data_binding:mark_changed()
   return self
end

function ConstructionDataComponent:set_tangent(tangent)
   self._data.tangent = tangent
   self._data_binding:mark_changed()
   return self
end

return ConstructionDataComponent
