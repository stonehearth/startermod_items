local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local TraceCategories = _radiant.dm.TraceCategories

local ConstructionDataRenderer = class()

local INFINITE = 1000000

function ConstructionDataRenderer:initialize(render_entity, construction_data)   
   self._entity = render_entity:get_entity()
   self._render_entity = render_entity
   self._parent_node = render_entity:get_node()
   self._construction_data = construction_data
   
   if not self._construction_data:get_use_custom_renderer() then

      -- create a new ConstructionRenderTracker to track the hud mode and build
      -- view mode.  it will drive the shape and visibility of our structure shape
      -- based on those modes.
      self._render_tracker = ConstructionRenderTracker(self._entity)
                                 :set_type(construction_data:get_type())
                                 :set_normal(construction_data:get_normal())
                                 :set_render_region_changed_cb(function(region, visible, mode)
                                       self:_update_region(region, mode)
                                    end)
                                 :set_visible_changed_cb(function(visible)
                                       self._render_entity:set_visible_override(visible)
                                    end)
                                 :start()

      self._construction_data_trace = construction_data:trace_data('render')
                                          :on_changed(function()
                                                self._render_tracker:push_visible_state()
                                             end)

      self._component_trace = self._entity:trace_components('render')
                                 :on_added(function(key, value)
                                       self:_trace_collision_shape()
                                    end)
                                 :push_object_state()
      self._mode_listener = radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
   end
end

function ConstructionDataRenderer:destroy()
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil   
   end
   if self._construction_data_trace then
      self._construction_data_trace:destroy()
      self._construction_data_trace = nil
   end
   if self._component_trace then
      self._component_trace:destroy()
      self._component_trace = nil
   end
   if self._collision_shape_trace then
      self._collision_shape_trace:destroy()
      self._collision_shape_trace = nil
   end
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if self._mode_listener then
      self._mode_listener:destroy()
      self._mode_listener = nil
   end
end

function ConstructionDataRenderer:_on_ui_mode_changed()
   stonehearth.selection:set_selectable(self._entity, stonehearth.renderer:get_ui_mode() ~= 'normal')
end

function ConstructionDataRenderer:_trace_collision_shape()
   self._collision_shape = self._entity:get_component('region_collision_shape')   
   if self._collision_shape then
      if self._collision_shape_trace then
         self._collision_shape_trace:destroy()
         self._collision_shape_trace = nil
      end
      self._collision_shape_trace = self._collision_shape:trace_region('drawing construction', TraceCategories.SYNC_TRACE)
                                             :on_changed(function ()
                                                   self._render_tracker:set_region(self._collision_shape:get_region())
                                                end)
                                             :push_object_state()
   end
end

function ConstructionDataRenderer:_update_region(region, mode) 
   -- recreate our render node
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if region then
      self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, self._construction_data)
   end

   -- if we enter or leave rpg mode, hide all our children that are not
   -- positioned at the bottom of the structure
   if mode ~= self._view_mode then
      self._view_mode = mode
      local ec = self._entity:get_component('entity_container')
      if ec then
         for _, child in ec:each_child() do
            self:_update_child_visibility(child)
         end
      end
   end
end

function ConstructionDataRenderer:_update_child_visibility(entity)
   -- we only want to manually toggle the project, so make sure we skip the
   -- blueprint and fabricator.
   if entity:get_component('stonehearth:construction_progress') or
      entity:get_component('stonehearth:fabricator') then
      return
   end
      
   local visible = true
   if self._view_mode == 'rpg' then
      local location = entity:get_component('mob'):get_location()
      visible = location.y == 0
   end

   _radiant.client.get_render_entity(entity)
                     :set_visible_override(visible)
end

return ConstructionDataRenderer

