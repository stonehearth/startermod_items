local StructureEditor = class()
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')

function StructureEditor.get_fbp_for(entity)
   if entity and entity:is_valid() then
      -- is this a fabricator?  if so, finding the blueprint and the project is easy!
      local fc = entity:get_component('stonehearth:fabricator')
      if fc then
         return entity, fc:get_blueprint(), fc:get_project()
      end

      -- is this a blueprint or project?  use the construction_data component
      -- to get back the fabricator
      local cd = entity:get_component('stonehearth:construction_data')
      if cd then
         return StructureEditor.get_fbp_for(cd:get_fabricator_entity())
      end
   end
end

function StructureEditor.get_fbp_for_structure(entity, structure_component_name)
   local fabricator, blueprint, project = StructureEditor.get_fbp_for(entity)
   if blueprint then
      local structure_component = blueprint:get_component(structure_component_name)
      if structure_component then
         return fabricator, blueprint, project, structure_component
      end
   end
end


function StructureEditor.get_building_for(entity)
   if entity and entity:is_valid() then
      local _, blueprint, _ = StructureEditor.get_fbp_for(entity)
      if blueprint then
         local cp = blueprint:get_component('stonehearth:construction_progress')
         if cp then
            return cp:get_building_entity()
         end
      end
   end
end

function StructureEditor:__init()
   self._building_container = radiant.entities.create_entity('stonehearth:entities:building')
   self._render_entity = _radiant.client.create_render_entity(1, self._building_container)
end

function StructureEditor:destroy()
   -- hide the fabricator and structure...
   self:_release_current_proxies()
   radiant.entities.destroy_entity(self._building_container)
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

   local building = StructureEditor.get_building_for(blueprint)
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
                           :set_region(_radiant.client.alloc_region())
   self._proxy_fabricator:add_component('destination')
                           :set_region(_radiant.client.alloc_region())
   self._proxy_fabricator:add_component('region_collision_shape')
                           :set_region(_radiant.client.alloc_region())

   self._proxy_blueprint:add_component('stonehearth:construction_data')
                           :begin_editing(self._blueprint)

   self._structure = self._proxy_blueprint:add_component(structure_type)
                                                :clone_from(self._blueprint)
                                                :begin_editing(self._blueprint)
                                                :layout()
   
   local editing_reserved_region = self._structure:get_editing_reserved_region()
   self._proxy_fabricator:add_component('stonehearth:fabricator')
         :begin_editing(self._proxy_blueprint, self._project, editing_reserved_region)

   self._proxy_blueprint:add_component('stonehearth:construction_progress')
                           :begin_editing(self._building_container, self._proxy_fabricator)


   -- hide the fabricator and structure...
   self:_show_editing_objects(false)
end

function StructureEditor:_show_editing_objects(visible)
   if self._fabricator and self._fabricator:is_valid() then
      _radiant.client.get_render_entity(self._fabricator)
                        :set_visible_override(visible)
   end
   if self._blueprint then
      _radiant.client.get_render_entity(self._project)
                        :set_visible_override(visible)
   end   
end

function StructureEditor:destroy()
   -- hide the fabricator and structure...
   radiant.entities.destroy_entity(self._proxy_fabricator)
   radiant.entities.destroy_entity(self._proxy_blueprint)
   radiant.entities.destroy_entity(self._building_container)
   self:_show_editing_objects(true)
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
