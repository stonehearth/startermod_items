local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local FabricatorRenderer = class()

-- keeps track of the currently selected building, globally.  used to
-- implement drawing the selected building in a different style
local selected_building
local all_buildings_map = {}

local function update_selected_building()
   local last_selected_building = selected_building
   local selected_entity = stonehearth.selection:get_selected_entity()

   if selected_entity then
      -- either the selected building is a building, itself, or something
      -- with a building pointer wired up.
      if selected_entity:get_uri() == 'stonehearth:entities:building' then
         selected_building = selected_entity
      else
         selected_building = all_buildings_map[selected_entity:get_id()]
      end
   else
      selected_building = nil
   end
   
   if selected_building then
      radiant.events.trigger(selected_building, 'stonehearth:building_selected_changed')
   end

   if last_selected_building then
      if not selected_building or selected_building:get_id() ~= last_selected_building:get_id() then
         radiant.events.trigger(last_selected_building, 'stonehearth:building_selected_changed')
      end
   end
end

radiant.events.listen(radiant, 'stonehearth:selection_changed', update_selected_building)

function FabricatorRenderer:__init(render_entity, datastore)
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()

   self._entity = render_entity:get_entity()
   self._parent_node = render_entity:get_node()
   self._render_entity = render_entity

   self._data = datastore:get_data()

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   radiant.events.listen(self._entity, 'stonehearth:selection_changed', self, self._update_render_state)
   radiant.events.listen(self._entity, 'stonehearth:hilighted_changed', self, self._update_render_state)
   
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
   end
   self:_update_ui_mode()
   self:_recreate_render_node()
end

function FabricatorRenderer:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   radiant.events.unlisten(self._entity, 'stonehearth:selection_changed', self, self._update_render_state)
   radiant.events.unlisten(self._entity, 'stonehearth:hilighted_changed', self, self._update_render_state)
   if self._building then
      radiant.events.unlisten(self._building, 'stonehearth:building_selected_changed', self, self._update_render_state)
   end
   
   if self._fab_trace then
      self._fab_trace:destroy()
      self._fab_trace = nil
   end
   if self._dst_trace then
      self._dst_trace:destroy()
      self._dst_trace = nil
   end
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
end

function FabricatorRenderer:_update_ui_mode()
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode
      self:_recreate_render_node()
   end
end

function FabricatorRenderer:_update_building()
   -- update our building pointer and subscribe to the stonehearth:building_selected_changed notification.
   -- walls and such don't move from building to building, so we only need to do this once.
   if not self._building then
      if self._blueprint_contruction_progress then
         local building = self._blueprint_contruction_progress:get_data().building_entity
         if building then
            self._building = building
            all_buildings_map[self._entity:get_id()] = building
            update_selected_building()
            radiant.events.listen(self._building, 'stonehearth:building_selected_changed', self, self._update_render_state)
         end
      end
   end
end

function FabricatorRenderer:_update_render_state()
   if self._render_node then

      self:_update_building()

      local material
      local entity_id = self._entity:get_id()
      local selected = stonehearth.selection:get_selected_id() == self._entity:get_id()
      local hovered = stonehearth.hilight:get_hilighted_id() == entity_id
      local building_selected = self._building and self._building == selected_building

      if selected then         
         material = 'selected'
      elseif hovered then
         material = building_selected and 'building_selected_hover' or 'hover'
      elseif building_selected then
         material = 'building_selected'
      else
         material = 'normal'
      end
      material = self._render_entity:get_material_path(material)
      self._render_node:set_material(material)
   end
end

function FabricatorRenderer:_recreate_render_node()
   self:_update_building()

   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end

   if self._ui_view_mode == 'hud' then
      local blueprint = self._data.blueprint
      if not self._blueprint_contruction_data then
         local construction_data = blueprint:get_component('stonehearth:construction_data')
         if construction_data then
            self._blueprint_contruction_data = construction_data
         end
      end
      if not self._blueprint_contruction_progress then
         local construction_progress = blueprint:get_component('stonehearth:construction_progress')
         if construction_progress then
            self._blueprint_contruction_progress = construction_progress
         end
      end
      if self._blueprint_contruction_data and self._blueprint_contruction_progress then
         local cp = self._blueprint_contruction_progress:get_data()
         if not cp.teardown then
            local cd = self._blueprint_contruction_data:get_data()
            if cd.brush then
               local region = self._destination:get_region()
               self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, cd, 'blueprint')
               self:_update_render_state()
            end
         end
      end
   end
end

return FabricatorRenderer