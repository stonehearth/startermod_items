local Constants = require 'services.build.constants'

local Proxy = class()
local Point3 = _radiant.csg.Point3

function Proxy:__init(derived, parent_proxy, arg1, component_name)
   self._derived = derived
   self._children = {}
   if type(arg1) == 'string' or not arg1 then
      self._entity = radiant.entities.create_entity(arg1)
      self._render_entity = _radiant.client.create_render_entity(1, self._entity)   
   else
      self._entity = arg1
   end
   self._component_name = component_name

   if parent_proxy then
      parent_proxy:add_child(derived)
   end
   
   -- proxies get rendered in blueprint
   self._entity:add_component('render_info')
                     :set_material('materials/blueprint_gridlines.xml')
                     :set_model_mode('blueprint')
end

function Proxy:destroy()
   for _, child in pairs(self._children) do
      child:destroy()
   end

   radiant.entities.destroy_entity(self._entity)
   self._render_entity = nil   
   
   return self._derived
end

function Proxy:move_to(location)
   self._entity:add_component('mob'):set_location_grid_aligned(location)
   return self._derived
end

function Proxy:turn_to(rotation)
   self._entity:add_component('mob'):turn_to(rotation)
   return self._derived
end

function Proxy:get_location()
   return self._entity:add_component('mob'):get_grid_location()
end

function Proxy:get_component_name()
   return self._component_name
end

function Proxy:get_entity()
   return self._entity
end

function Proxy:get_parent()
   return self._parent
end

function Proxy:_set_parent(parent)
   self._parent = parent
end

function Proxy:add_child(proxy)
   local parent = proxy:get_parent()
   if parent then
      parent:remove_child(proxy)
   end
   
   local entity = proxy:get_entity()
   proxy:_set_parent(self._derived)
   self._entity:add_component('entity_container'):add_child(entity)
   self._children[entity:get_id()] = proxy
end

function Proxy:remove_child(proxy)
   local entity = proxy:get_entity()
   local entity_id = entity:get_id()
   if self._children[entity_id] then
      proxy:_set_parent(nil)
      self._entity:add_component('entity_container'):remove_child(entity)
      self._children[entity_id] = nil
   end
end

function Proxy:get_children()
   return self._children
end

return Proxy
