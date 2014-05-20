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
      local cp = blueprint:get_component('stonehearth:construction_progress')
      if cp then
         return cp:get_building_entity()
      end
   end
end

function StructureEditor:__init(fabricator, blueprint, project, structure_type)
   self._fabricator = fabricator
   self._blueprint = blueprint
   self._project = project

   local building = get_building_for(blueprint)
   local location = radiant.entities.get_world_grid_location(building)
   self._proxy_building = radiant.entities.create_entity(building:get_uri())
   self._proxy_building:set_debug_text('proxy building')
   self._proxy_building:add_component('mob')
                           :set_location_grid_aligned(location)
   blueprint:add_component('unit_info')
            :set_player_id(radiant.entities.get_player_id(building))
            :set_faction(radiant.entities.get_faction(building))

   local rgn = _radiant.client.alloc_region()
   rgn:modify(function(cursor)
         local source_rgn = blueprint:get_component('destination'):get_region()
         cursor:copy_region(source_rgn:get())
      end)
   self._proxy_blueprint = radiant.entities.create_entity(blueprint:get_uri())
   self._proxy_blueprint:set_debug_text('proxy blueprint')
   self._proxy_blueprint:add_component('destination')
                           :set_region(rgn)

   local structure = blueprint:get_component(structure_type)
   assert(structure)
   local proxy_structure = self._proxy_blueprint:add_component(structure_type)
                                                   :begin_editing(structure)
                                                   :layout()
   local editing_reserved_region = proxy_structure:get_editing_reserved_region()
   
   self._proxy_blueprint:add_component('stonehearth:construction_data')
                           :begin_editing(blueprint:get_component('stonehearth:construction_data'))

   self._proxy_fabricator = radiant.entities.create_entity(fabricator:get_uri())
   self._proxy_fabricator :set_debug_text('proxy fabricator')

   rgn = _radiant.client.alloc_region()
   rgn:modify(function(cursor)
         local source_rgn = fabricator:get_component('destination'):get_region()
         cursor:copy_region(source_rgn:get())
      end)
   self._proxy_fabricator:add_component('destination')
                           :set_region(rgn)
   
   self._proxy_fabricator:add_component('stonehearth:fabricator')
      :begin_editing(self._proxy_blueprint, project, editing_reserved_region)

   self._proxy_blueprint:add_component('stonehearth:construction_progress')
                           :begin_editing(self._proxy_building, self._proxy_fabricator)

   radiant.entities.add_child(self._proxy_building, self._proxy_blueprint, blueprint:get_component('mob'):get_grid_location())
   radiant.entities.add_child(self._proxy_building, self._proxy_fabricator, fabricator:get_component('mob'):get_grid_location() + Point3(0, 0, 0))

   self._render_entity = _radiant.client.create_render_entity(1, self._proxy_building)

   -- hide the fabricator and structure...
   _radiant.client.get_render_entity(self._fabricator):set_visible(false)
   _radiant.client.get_render_entity(self._project):set_visible(false)
end

function StructureEditor:destroy()
   -- hide the fabricator and structure...
   radiant.entities.destroy_entity(self._proxy_fabricator)
   radiant.entities.destroy_entity(self._proxy_blueprint)
   radiant.entities.destroy_entity(self._proxy_building)

   _radiant.client.get_render_entity(self._fabricator):set_visible(true)
   _radiant.client.get_render_entity(self._project):set_visible(true)
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
