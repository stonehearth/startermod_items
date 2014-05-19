local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
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

   if last_selected_building and last_selected_building:is_valid() then
      if not selected_building or selected_building ~= last_selected_building then
         radiant.events.trigger(last_selected_building, 'stonehearth:building_selected_changed')
      end
   end
end

radiant.events.listen(radiant, 'stonehearth:selection_changed', update_selected_building)

function FabricatorRenderer:initialize(render_entity, fabricator)
   self._datastore = fabricator.__saved_variables
   self._ui_view_mode = stonehearth.renderer:get_ui_mode()

   self._entity = render_entity:get_entity()
   self._parent_node = render_entity:get_node()
   self._render_entity = render_entity

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   radiant.events.listen(self._entity, 'stonehearth:selection_changed', self, self._update_render_state)
   radiant.events.listen(self._entity, 'stonehearth:hilighted_changed', self, self._update_render_state)
   
   self._fab_trace = self._datastore:trace_data('rendering a fabrication')
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
   if self._blueprint_dst_trace then
      self._blueprint_dst_trace:destroy()
      self._blueprint_dst_trace = nil
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
         local building = self._blueprint_contruction_progress:get_building_entity()
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
   self._sv = self._datastore:get_data()

   self:_update_building()

   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end

   if self._ui_view_mode == 'hud' then
      local blueprint = self._sv.blueprint
      if not self._blueprint_contruction_data then
         self._blueprint_contruction_data = blueprint:get_component('stonehearth:construction_data')
      end
      if not self._blueprint_contruction_progress then
         self._blueprint_contruction_progress = blueprint:get_component('stonehearth:construction_progress')
      end
      if not self._blueprint_dst_trace then
         local dst = blueprint:get_component('destination')
         if dst then
            assert(not self._blueprint_dst_trace)
            self._blueprint_dst_trace = dst:trace_region('render fab')
                        :on_changed(function(r)
                              self:_on_blueprint_collision_shape_changed(r)
                           end)
                        :push_object_state()
         end
      end
      if self._blueprint_contruction_data and self._blueprint_contruction_progress then
         local cd = self._blueprint_contruction_data
         if not self._blueprint_contruction_progress:get_teardown() then
            local region = self._destination:get_region()
            self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, cd, 'blueprint')
            
            -- columns are `connected_to` walls.  don't draw arrows for columns!
            local normal = cd:get_normal()
            if not cd:get_connected_to() and normal and self._blueprint_collision_bounds then
               if self._arrow_render_object then
                  self._arrow_render_object:destroy()
                  self._arrow_render_object = nil
               end
               self._arrow_render_object = _radiant.client.create_obj_render_node(self._render_node, 'stonehearth:models:arrow')
               
               local t, n, width
               -- the arrow should span the entire base of the blueprint
               if normal.x == 0 then
                  t, n = 'x', 'z'
               else
                  t, n = 'z', 'x'
               end
               
               width = self._blueprint_collision_bounds.max[t] - self._blueprint_collision_bounds.min[t]
               local pos = (self._blueprint_collision_bounds.min + normal):to_float()
               pos[t] = pos[t] + (width / 2) - 0.5

               -- padding when possible
               width = math.max(width - 2, 1)
               
               local rotation = voxel_brush_util.normal_to_rotation(normal)
               
               self._arrow_render_object:set_position(pos)
                                        :set_rotation(Point3f(0, rotation, 0))
                                        :set_scale(Point3f(width, 0.1, 1)) -- scale is applied after rotation, so always scale in x
                                        :set_material('materials/building_widget_arrow.material.xml')
            end
            self:_update_render_state()
         end
      end
   end
end

function FabricatorRenderer:_on_blueprint_collision_shape_changed(region)
   if region then
      self._blueprint_collision_bounds = region:get_bounds()
   else
      self._blueprint_collision_bounds = nil
   end
end

return FabricatorRenderer
