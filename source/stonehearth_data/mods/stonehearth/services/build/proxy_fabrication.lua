local Proxy = require 'services.build.proxy'
local ProxyFabrication = class(Proxy)
local Point3 = _radiant.csg.Point3

function ProxyFabrication:__init(derived,parent_proxy, arg1, component_name)
   self[Proxy]:__init(derived, parent_proxy, arg1, component_name)

   local entity = self:get_entity()
   
   self._rgn = _radiant.client.alloc_region()
   entity:add_component('destination')
                        :set_region(self._rgn)
   entity:add_component('region_collision_shape')
                        :set_region(self._rgn)

   self._data = entity:get_component_data(component_name)
   self:_update_datastore()
end

function ProxyFabrication:get_region()
   return self._rgn
end

function ProxyFabrication:get_normal()
   return self._normal
end

function ProxyFabrication:set_normal(normal)
   self._normal = normal
   self:_update_datastore()
   return self._derived
end

function ProxyFabrication:get_tangent()
   return self._tangent
end

function ProxyFabrication:set_tangent(tangent)
   self._tangent = tangent
   self:_update_datastore()
   return self._derived
end

function ProxyFabrication:get_voxel_brush()
   local normal = self:get_normal()
   local fabinfo = radiant.entities.get_entity_data(self:get_entity(), 'stonehearth:voxel_brush_info')
   assert(fabinfo, 'wall is missing stonehearth:voxel_brush_info invariant')
   
   local brush = _radiant.voxel.create_brush(fabinfo.brush)
                        :set_normal(normal)
                        :set_paint_mode(_radiant.voxel.QubicleBrush.Opaque)
   return brush
end

function ProxyFabrication:extend(json)
   if json.normal then
      self._normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   if json.tangent then
      self._tangent = Point3(json.tangent.x, json.tangent.y, json.tangent.z)
   end
   self:_update_datastore()
   return self._derived
end

function ProxyFabrication:_update_datastore()
   self._data.tangent = self._tangent
   self._data.normal = self._normal
   self._data.paint_mode = 'blueprint'
   
   self:get_entity():set_component_data(self:get_component_name(), self._data)
end

return ProxyFabrication
