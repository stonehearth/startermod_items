local Scaffolding = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- xxx: it would be nice if we could update the datastore once at an appropriate
-- time in the gameloop instead of everytime the normal or tangent changes.
function Scaffolding:__init(entity, data_binding)
   self._entity = entity
   self._data_binding = data_binding
   self:_update_datastore()
end

function Scaffolding:extend(json)
   if json.normal then
      self._normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   self:_update_datastore()
end

function Scaffolding:set_normal(normal)
   self._normal = normal
   self:_update_datastore()
   return self
end

function Scaffolding:_update_datastore()
   self._data_binding:update({
      normal = self._normal,
      project_adjacent_to_base = true,
      needs_scaffolding = false
   })
end

return Scaffolding
