local build_util = require 'lib.build_util'

local StructureEditor = class()
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')

function StructureEditor:__init()
   self._building_container = radiant.entities.create_entity('stonehearth:entities:building')
   radiant.entities.add_child(radiant._authoring_root_entity, self._building_container)
   self._render_entity = _radiant.client.create_render_entity(H3DRootNode, self._building_container)
end

function StructureEditor:destroy()
   -- hide the fabricator and structure...
   self:_release_current_proxies()

   radiant.entities.destroy_entity(self._building_container)

   if self._fabriator_visibility_handle then
      self._fabriator_visibility_handle:destroy()
      self._fabriator_visibility_handle = nil
   end

   if self._project_visibility_handle then
      self._project_visibility_handle:destroy()
      self._project_visibility_handle = nil
   end
end

function StructureEditor:_release_current_proxies()
   self:_show_editing_objects(true)
   self._fabricator = nil
   self._blueprint = nil
   self._project = nil

   if self._proxy_fabricator then
      radiant.entities.destroy_entity(self._proxy_fabricator)
      self._proxy_fabricator = nil
   end
   if self._proxy_blueprint then
      radiant.entities.destroy_entity(self._proxy_blueprint)
      self._proxy_blueprint = nil
   end
end

function StructureEditor:begin_editing(fabricator, blueprint, project, structure_type)
   self:_release_current_proxies()

   self._fabricator = fabricator
   self._blueprint = blueprint
   self._project = project

   self._fabriator_visibility_handle = self:_get_visibility_handle(self._fabricator)
   self._project_visibility_handle = self:_get_visibility_handle(self._project)

   local building = build_util.get_building_for(blueprint)
   --TODO: why is this sometimes nil?   
   if building then 
      local location = radiant.entities.get_world_grid_location(building)
      radiant.entities.move_to(self._building_container, location)
   end
   
   self:_initialize_proxies(blueprint:get_uri(), structure_type)

   local location = blueprint:get_component('mob'):get_grid_location()
   self:move_to(location)
end

function StructureEditor:create_blueprint(blueprint_uri, structure_type)
   self:_initialize_proxies(blueprint_uri, structure_type)
   return self._proxy_blueprint
end

function StructureEditor:move_to(location)
   radiant.entities.move_to(self._proxy_blueprint, location)
   radiant.entities.move_to(self._proxy_fabricator, location)
end


function StructureEditor:_initialize_proxies(blueprint_uri, structure_type)
   self._proxy_blueprint = radiant.entities.create_entity(blueprint_uri)
   self._proxy_fabricator = radiant.entities.create_entity('stonehearth:entities:fabricator')

   radiant.entities.add_child(self._building_container, self._proxy_blueprint)
   radiant.entities.add_child(self._building_container, self._proxy_fabricator)

   self._proxy_blueprint:add_component('destination')
                           :set_region(_radiant.client.alloc_region3())
   self._proxy_fabricator:add_component('destination')
                           :set_region(_radiant.client.alloc_region3())
   self._proxy_fabricator:add_component('region_collision_shape')
                           :set_region(_radiant.client.alloc_region3())

   self._structure = self._proxy_blueprint:add_component(structure_type)

   if self._blueprint then
      self._proxy_blueprint:add_component('stonehearth:construction_data')
                              :begin_editing(self._blueprint)

      self._structure:clone_from(self._blueprint)
                     :begin_editing(self._blueprint)
                     :layout()
   end

   local editing_reserved_region = self._structure:get_editing_reserved_region()
   self._proxy_fabricator:add_component('stonehearth:fabricator')
                           :begin_editing(self._proxy_blueprint, self._project, editing_reserved_region)

   self._proxy_blueprint:add_component('stonehearth:construction_progress')
                           :begin_editing(self._building_container, self._proxy_fabricator)
   -- hide the fabricator and structure...
   self:_show_editing_objects(false)
end

function StructureEditor:_get_visibility_handle(entity)
   local render_entity = _radiant.client.get_render_entity(entity)
   return render_entity and render_entity:get_visibility_override_handle()
end

function StructureEditor:_show_editing_objects(visible)
   if self._fabricator and self._fabricator:is_valid() then
      self._fabriator_visibility_handle:set_visible(visible)
   end
   if self._blueprint then
      self._project_visibility_handle:set_visible(visible)
   end   
end

function StructureEditor:get_blueprint()
   return self._blueprint
end

function StructureEditor:get_proxy_blueprint()
   return self._proxy_blueprint
end

function StructureEditor:get_proxy_fabricator()
   return self._proxy_fabricator
end

function StructureEditor:get_world_origin()
   return radiant.entities.get_world_grid_location(self._proxy_fabricator)
end

function StructureEditor:should_keep_focus(entity)
   if entity == self._proxy_blueprint or 
      entity == self._proxy_fabricator or 
      entity == self._project then
      return true
   end
-- keep going up until we find one!
   local mob = entity:get_component('mob')
   if mob then
      local parent = mob:get_parent()
      if parent then
         log:detail('checking parent... %s', parent)
         return self:should_keep_focus(parent)
      end
   end
   return false
end

return StructureEditor
