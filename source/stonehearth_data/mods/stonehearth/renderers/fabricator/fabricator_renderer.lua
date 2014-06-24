local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local TraceCategories = _radiant.dm.TraceCategories

local FabricatorRenderer = class()

local MODEL_OFFSET = Point3f(0.5, 0, 0.5)

function FabricatorRenderer:initialize(render_entity, fabricator)
   self._entity = render_entity:get_entity()
   self._parent_node = render_entity:get_node()
   self._render_entity = render_entity

   local blueprint = fabricator:get_data().blueprint
   assert(blueprint)

   self._blueprint_cd = blueprint:get_component('stonehearth:construction_data')
   self._blueprint_cp = blueprint:get_component('stonehearth:construction_progress')
   assert(self._blueprint_cd)
   assert(self._blueprint_cp)

   if not self._blueprint_cd:get_use_custom_renderer() then

      -- create a new ConstructionRenderTracker to track the hud mode and build
      -- view mode.  it will drive the shape and visibility of our structure shape
      -- based on those modes.  
      self._render_tracker = ConstructionRenderTracker(self._entity)
                                 :set_type(self._blueprint_cd:get_type())
                                 :set_normal(self._blueprint_cd:get_normal())
                                 :set_visible_ui_modes('hud')
                                 :set_render_region_changed_cb(function(region, visible)
                                       self:_update_region(region, visible)
                                    end)
                                 :set_visible_changed_cb(function(visible)
                                       if self._render_node then
                                          self._render_node:set_visible(visible)
                                       end
                                    end)

      self._construction_data_trace = self._blueprint_cd:trace_data('render')
                                          :on_changed(function()
                                                self._render_tracker:push_visible_state()
                                             end)

      -- push the destination region into the render tracker whenever it changes
      self._destination = self._entity:get_component('destination')
      assert(self._destination)
      if self._destination then
         local region = self._destination:get_region()
         self._dst_trace = self._destination:trace_region('drawing fabricator', TraceCategories.SYNC_TRACE)
                                                   :on_changed(function ()
                                                      self._render_tracker:set_region(region)
                                                   end)
                                                   :push_object_state()
      end

      -- the editing region exists on client-side created entities to make sure we intercept
      -- all raycasts which might otherwise go to other objects (e.g. when moving a window
      -- around and the mouse is temporarily over the hole)
      self._editing_region = fabricator:get_editing_region()
      if self._editing_region then
         self._editing_region_trace = self._editing_region:trace('render', TraceCategories.SYNC_TRACE)
            :on_changed(function()
                  self:_recreate_editing_region_node()
               end)
            :push_object_state()
      end
   end
end

function FabricatorRenderer:destroy()
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil   
   end
   if self._construction_data_trace then
      self._construction_data_trace:destroy()
      self._construction_data_trace = nil
   end
   if self._editing_region_node then
      self._editing_region_node:destroy()
      self._editing_region_node = nil
   end   
   if self._editing_region_trace then
      self._editing_region_trace:destroy()
      self._editing_region_trace = nil
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

function FabricatorRenderer:_update_region(region, visible)
   if self._render_node then
      self._render_node:destroy()
      self._render_node = nil
   end

   if not self._blueprint_cp:get_teardown() and region then
      self._render_node = voxel_brush_util.create_construction_data_node(self._parent_node, self._entity, region, self._blueprint_cd)
      self._render_node:set_name(string.format('fab for %s', tostring(self._entity)))

      local material = self._render_entity:get_material_path('normal')
      self._render_node:set_material(material)
      self._render_node:set_visible(visible)
   end
end

function FabricatorRenderer:_recreate_editing_region_node()
   if self._editing_region_node then
      self._editing_region_node:destroy()
      self._editing_region_node = nil
   end
   self._editing_region_node = _radiant.client.create_voxel_node(self._parent_node, self._editing_region:get(), '', MODEL_OFFSET)
                                                :set_name(string.format('editing region for %s', tostring(self._entity)))
                                                :set_visible(false)
                                                :set_can_query(true)
end

return FabricatorRenderer
