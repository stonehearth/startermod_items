local BuildComponent = class()
local Point3 = _radiant.csg.Point3

function BuildComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self.STOREY_HEIGHT = 6

   self._data = self._data_binding:get_data()
end

function BuildComponent:get_region()
   return self._rgn
end

function BuildComponent:get_normal()
   return self._data.normal
end

function BuildComponent:set_normal(normal)
   self._data.normal = normal
   self._data_binding:mark_changed()
end

function BuildComponent:extend(json)
   if json.normal then
      self._data.normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   self._data_binding:mark_changed()
end

return BuildComponent
