local BuildComponent = class()
local Point3 = _radiant.csg.Point3

function BuildComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self.STOREY_HEIGHT = 6

   self._rgn = _radiant.sim.alloc_region()   
   entity:add_component('destination')
                     :set_region(self._rgn)
   entity:add_component('region_collision_shape')
                     :set_region(self._rgn)
   
end

function BuildComponent:get_region()
   return self._rgn
end

function BuildComponent:get_normal()
   return self._normal
end

function BuildComponent:set_normal(normal)
   self._normal = normal
   self:_update_datastore()
end

function BuildComponent:get_tangent()
   return self._tangent
end

function BuildComponent:set_tangent(tangent)
   self._tangent = tangent
   self:_update_datastore()
end

function BuildComponent:extend(json)
   if json.normal then
      self._normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   if json.tangent then
      self._tangent = Point3(json.tangent.x, json.tangent.y, json.tangent.z)
   end
   self:_update_datastore() 
end

function BuildComponent:_update_datastore()
   self._data_binding:update({
      normal = self._normal,
      tangent = self._tangent,
      project_adjacent_to_base = false,
      needs_scaffolding = true
   })
end

return BuildComponent
