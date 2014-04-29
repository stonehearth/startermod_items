local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local FabricatorRenderer = class()

function FabricatorRenderer:__init()
   radiant.events.listen(radiant.events, 'stonehearth:ui_mode_changed', function(e)
      local mode = stonehearth.renderer:get_ui_mode()
      if self._ui_view_mode ~= mode then
         self._ui_view_mode = mode
         self:_update()
      end
   end)  
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()
end

function FabricatorRenderer:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:selection_changed', self, self._update_color)
   radiant.events.unlisten(self._entity, 'stonehearth:hilighted_changed', self, self._update_color)
end

function FabricatorRenderer:update(render_entity, datastore)
   self._parent_node = render_entity:get_node()
   self._entity = render_entity:get_entity()
   self._render_entity = render_entity
   radiant.events.listen(self._entity, 'stonehearth:selection_changed', self, self._update_color)
   radiant.events.listen(self._entity, 'stonehearth:hilighted_changed', self, self._update_color)
   
   self._data = datastore:get_data()
   self._data_store = datastore
   self._ds_promise = datastore:trace_data('rendering a fabrication')
                        :on_changed(function()
                           self:_update()
                        end)

   self._destination = self._entity:get_component('destination')
   if self._destination then
      self._cs_promise = self._destination:trace_region('drawing fabricator')
                                                :on_changed(function ()
                                                   self:_update()
                                                end)
      self:_update()
   end
   self:_update()
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

function FabricatorRenderer:_update()
   if self._unique_renderable then
      self._unique_renderable:destroy()
      self._unique_renderable = nil
   end

   if self:_show_hud() then
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

function FabricatorRenderer:_show_hud()
   return self._ui_view_mode == 'hud'
end


function FabricatorRenderer:destroy()
end

return FabricatorRenderer