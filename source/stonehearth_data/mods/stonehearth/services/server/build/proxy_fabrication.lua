local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local Proxy = require 'services.server.build.proxy'
local ProxyFabrication = class(Proxy)
local Point3 = _radiant.csg.Point3

function ProxyFabrication:__init(derived,parent_proxy, arg1, component_name)
   self[Proxy]:__init(derived, parent_proxy, arg1, component_name)

   local entity = self:get_entity()
   
   self._rgn = _radiant.client.alloc_region()
   entity:add_component('destination')
                        :set_region(self._rgn)
                        :set_auto_update_adjacent(true)
   entity:add_component('region_collision_shape')
                        :set_region(self._rgn)

   self:add_construction_data()
end

function ProxyFabrication:get_world_location()
   return self:get_entity():add_component('mob'):get_world_grid_location()
end

function ProxyFabrication:get_region()
   return self._rgn
end

function ProxyFabrication:get_normal()
   return self._normal
end

function ProxyFabrication:set_normal(normal)
   self._normal = normal

   local data = self:add_construction_data()
   data:modify_data(function(o)
         o.normal = normal
      end)

   return self._derived
end

function ProxyFabrication:get_voxel_brush()
   local datastore = self:get_construction_data()
   assert(datastore, 'fabrication is missing stonehearth:construction component')

   return voxel_brush_util.create_brush(datastore:get_data())
end

function ProxyFabrication:initialize(entity, json)
   if json.normal then
      local normal = Point3(json.normal.x, json.normal.y, json.normal.z)
      self:set_normal(normal)
   end
   return self._derived
end

return ProxyFabrication
