local Constants = require 'services.build.constants'

local ProxyFabrication = class()
local Point3 = _radiant.csg.Point3

function ProxyFabrication:__init(arg1, component_name)
   if type(arg1) == 'string' then
      self._entity = radiant.entities.create_entity(arg1)
      self._render_entity = _radiant.client.create_render_entity(1, self._entity)   
   else
      self._entity = arg1
   end
   self._component_name = component_name

   self._rgn = _radiant.client.alloc_region()  
   self._entity:add_component('destination')
                        :set_region(self._rgn)
   self._entity:add_component('region_collision_shape')
                        :set_region(self._rgn)
                        
   -- proxies get rendered in blueprint
   self._entity:add_component('render_info')
                  :set_material('materials/blueprint_gridlines.xml')
                  
   self:_update_datastore()
end

function ProxyFabrication:move_to(location)
   self._entity:add_component('mob'):set_location_grid_aligned(location)
end

function ProxyFabrication:get_location()
   return self._entity:add_component('mob'):get_world_grid_location()
end

function ProxyFabrication:get_component_name()
   return self._component_name
end

function ProxyFabrication:get_entity()
   return self._entity
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
end

function ProxyFabrication:get_tangent()
   return self._tangent
end

function ProxyFabrication:set_tangent(tangent)
   self._tangent = tangent
   self:_update_datastore()
end

function ProxyFabrication:extend(json)
   if json.normal then
      self._normal = Point3(json.normal.x, json.normal.y, json.normal.z)
   end
   if json.tangent then
      self._tangent = Point3(json.tangent.x, json.tangent.y, json.tangent.z)
   end
   self:_update_datastore() 
end

function ProxyFabrication:_update_datastore()
   local data = {
      normal = self._normal,
      tangent = self._tangent,
      paint_mode = "blueprint",
      project_adjacent_to_base = false,
      needs_scaffolding = true
   }
   self._entity:set_component_data(self._component_name, data)
end

return ProxyFabrication
