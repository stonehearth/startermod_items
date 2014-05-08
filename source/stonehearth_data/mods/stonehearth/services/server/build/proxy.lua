-- move to the client! -- tony
local voxel_brush_util = require 'services.server.build.voxel_brush_util'

local Proxy = class()
local Point3 = _radiant.csg.Point3

function Proxy:__init(derived, parent_proxy, uri_or_entity)
   self._derived = derived
   self._children = {}
   self._dependencies = {}
   self._loaning_scaffolding_to = {}

   if type(uri_or_entity) == 'string' or not uri_or_entity then
      self._entity = radiant.entities.create_entity(uri_or_entity)
      self._render_entity = _radiant.client.create_render_entity(1, self._entity)
   else
      self._entity = uri_or_entity
   end

   if parent_proxy then
      parent_proxy:add_child(derived)
   end

   self._construction_data = self._entity:get_component('stonehearth:construction_data')
   if self._construction_data then
      self._construction_data:modify_data(function(data)
            data.paint_mode = "blueprint"
         end)
   end
   -- proxies get rendered in blueprint
   self._entity:add_component('render_info')
                     :set_material('materials/blueprint_gridlines.xml')
end

function Proxy:destroy()
   for _, child in pairs(self._children) do
      child:destroy()
   end

   radiant.entities.destroy_entity(self._entity)
   self._render_entity = nil   
   
   return self._derived
end

function Proxy:get_id()
   return self._entity:get_id()
end

function Proxy:move_to(location)
   self._entity:add_component('mob'):set_location_grid_aligned(location)
   return self._derived
end

function Proxy:move_to_absolute(location)
   local mob = self._entity:add_component('mob')
   local parent = mob:get_parent()
   if parent then
      location = location - parent:add_component('mob'):get_world_grid_location()
   end
   self:move_to(location)   
end

function Proxy:turn_to(rotation)
   self._entity:add_component('mob'):turn_to(rotation)
   return self._derived
end

function Proxy:get_location()
   return self._entity:add_component('mob'):get_grid_location()
end
function Proxy:get_location_absolute()
   return self._entity:add_component('mob'):get_world_grid_location()
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
      self._entity:add_component('entity_container'):remove_child(entity_id)
      self._children[entity_id] = nil
   end
end

function Proxy:get_children()
   return self._children
end

function Proxy:get_dependencies()
   return self._dependencies
end

function Proxy:get_building()
   return self._building
end

function Proxy:set_building(building)
   self._building = building
   return self._derived
end

function Proxy:add_dependency(dependency)
   local id = dependency:get_id()
   self._dependencies[id] = dependency
   return self._derived
end

function Proxy:loan_scaffolding_to(borrower)
   local id = borrower:get_id()
   self._loaning_scaffolding_to[id] = borrower
   return self._derived
end

function Proxy:get_loaning_scaffolding_to(borrower)
   return self._loaning_scaffolding_to
end

function Proxy:get_construction_data()
   return self._construction_data
end

function Proxy:add_construction_data()
   if not self._construction_data then
      local data = {
         paint_mode = 'blueprint'
      }
      self._construction_data = self._entity:add_component_data('stonehearth:construction_data', data)
   end
   return self._construction_data
end


return Proxy
