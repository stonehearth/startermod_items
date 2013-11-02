local voxel_brush_util = require 'services.build.voxel_brush_util'

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

   self:add_construction_data()
end

function ProxyFabrication:get_region()
   return self._rgn
end

function ProxyFabrication:get_normal()
   return self._normal
end

function ProxyFabrication:get_tangent()
   return self._tangent
end

function ProxyFabrication:set_normal(normal)
   self._normal = normal
   self:add_construction_data().normal = normal
   self:update_datastore()
   return self._derived
end

function ProxyFabrication:set_tangent(tangent)
   self._tangent = tangent
   self:add_construction_data().tangent = tangent
   self:update_datastore()
   return self._derived
end

function ProxyFabrication:get_voxel_brush()
   local construction_data = self:get_construction_data()
   assert(construction_data, 'fabrication is missing stonehearth:construction component')

   return voxel_brush_util.create_brush(construction_data)
end

function ProxyFabrication:extend(json)
   if json.normal then
      self._normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   if json.tangent then
      self._tangent = Point3(json.tangent.x, json.tangent.y, json.tangent.z)
   end
   self:update_datastore()
   return self._derived
end


return ProxyFabrication
