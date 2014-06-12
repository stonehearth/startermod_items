local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local ConstructionDataRenderer = class()

local INFINITE = 1000000

function ConstructionDataRenderer:initialize(render_entity, construction_data)
   self._entity = render_entity:get_entity()
   self._parent_node = render_entity:get_node()
   self._construction_data = construction_data
   
   if not self._construction_data:get_use_custom_renderer() then

      -- create a new ConstructionRenderTracker to track the hud mode and build
      -- view mode.  it will drive the shape and visibility of our structure shape
      -- based on those modes.
      self._render_tracker = ConstructionRenderTracker(self._entity)
                                 :set_normal(construction_data:get_normal())
                                 :set_render_region_changed_cb(function(region, visible)
                                       self:_update_region(region, visible)
                                    end)
                                 :set_visible_changed_cb(function(visible)
                                       if self._render_node then
                                          self._render_node:set_visible(visible)
                                       end
                                    end)

      self._construction_data_trace = construction_data:trace_data('render')
                                          :on_changed(function()
                                                self._render_tracker:push_visible_state()
                                             end)

      self._component_trace = self._entity:trace_components('render')
                                 :on_added(function(key, value)
                                       self:_trace_collision_shape()
                                    end)
                                 :push_object_state()
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
end

function ConstructionDataRenderer:_trace_collision_shape()
   self._collision_shape = self._entity:get_component('region_collision_shape')   
   if self._collision_shape then
      if self._collision_shape_trace then
         self._collision_shape_trace:destroy()
         self._collision_shape_trace = nil
      end
      self._collision_shape_trace = self._collision_shape:trace_region('drawing construction')
                                             :on_changed(function ()
                                                   self._render_tracker:set_region(self._collision_shape:get_region())
                                                end)
                                             :push_object_state()
   end
end

function ConstructionDataRenderer:_update_region(region, visible)
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end
   if region then   
      self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, self._construction_data)
      self._render_node:set_visible(visible)
   end
end

return ConstructionDataRenderer

