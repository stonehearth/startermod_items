local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local FabricatorRenderer = class()

function FabricatorRenderer:__init(render_entity, datastore)
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()

   self._entity = render_entity:get_entity()
   self._parent_node = render_entity:get_node()
   self._render_entity = render_entity

   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   radiant.events.listen(self._entity, 'stonehearth:selection_changed', self, self._update_color)
   radiant.events.listen(self._entity, 'stonehearth:hilighted_changed', self, self._update_color)
   
   self._data = datastore:get_data()

   self._fab_trace = datastore:trace_data('rendering a fabrication')
                        :on_changed(function()
                           self:_recreate_render_node()
                        end)

   -- xxx: don't we need to install a component trace to see if the destination changes?
   self._destination = self._entity:get_component('destination')
   if self._destination then
      self._dst_trace = self._destination:trace_region('drawing fabricator')
                                                :on_changed(function ()
                                                   self:_recreate_render_node()
                                                end)
      self:_recreate_render_node()
   end
   self:_recreate_render_node()   
end

function FabricatorRenderer:destroy()
   radiant.events.unlisten(radiant.events, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   radiant.events.unlisten(self._entity, 'stonehearth:selection_changed', self, self._update_color)
   radiant.events.unlisten(self._entity, 'stonehearth:hilighted_changed', self, self._update_color)
   
   if self._fab_trace then
      self._fab_trace:destroy()
      self._fab_trace = nil
   end
   if self._dst_trace then
      self._dst_trace:destroy()
      self._dst_trace = nil
   end
   if self._unique_renderable then
      self._unique_renderable:destroy()
      self._unique_renderable = nil
   end
end

function FabricatorRenderer:_update_ui_mode()
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode
      self:_recreate_render_node()
   end
end

function FabricatorRenderer:_update_color()
   local material = ''
   local entity_id = self._entity:get_id()
   local selection_id = stonehearth.selection:get_selected_id()
   if selection_id == entity_id then
      material = 'selected'
   else
      local hilighted_id = stonehearth.hilight:get_hilighted_id()
      if hilighted_id == entity_id then
         material = 'hover'
      end
   end
   self._render_entity:set_material_override(material)
end

function FabricatorRenderer:_recreate_render_node()
   if self._unique_renderable then
      self._unique_renderable:destroy()
      self._unique_renderable = nil
   end

   if self._ui_view_mode == 'hud' then
      local blueprint = self._data.blueprint
      local component_data = blueprint:get_component('stonehearth:construction_data')
      if component_data then
         local construction_data = component_data:get_data()
         if construction_data.brush then
            local region = self._destination:get_region()
            self._unique_renderable = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, construction_data, 'blueprint')
         end
      end
   end
end

return FabricatorRenderer