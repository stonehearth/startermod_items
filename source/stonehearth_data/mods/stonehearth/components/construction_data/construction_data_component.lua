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

function ConstructionDataComponent:get_data()
   return self._data
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

function ConstructionDataComponent:set_finished(finished)
   self._data.finished = finished
   self._data_binding:mark_changed()
   return self
end

function ConstructionDataComponent:get_finished()
   return self._data.finished
end

function ConstructionDataComponent:get_allow_diagonal_adjacency()
   -- coearse to bool 
   return self._data.allow_diagonal_adjacency and true or false
end

function ConstructionDataComponent:get_project_adjacent_to_base()
   -- coearse to bool 
   return self._data.project_adjacent_to_base and true or false
end

return ConstructionDataComponent
