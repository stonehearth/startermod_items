local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Point3f = _radiant.csg.Point3f
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories

local LadderRenderer = class()
local log = radiant.log.create_logger('ladder.renderer')

function LadderRenderer:initialize(render_entity, ladder)
   self._entity = render_entity:get_entity()
   self._entity_node = render_entity:get_node()
   self._node = h3dAddGroupNode(self._entity_node, 'ladder node')
   
   self._ladder = ladder
   self._ladder_nodes = {}
   self._rotation = 0

   -- Pull some render parameters out of the entity data
   local ed = radiant.entities.get_entity_data(self._entity, 'stonehearth:ladder_render_info')
   self._lattice = ed.lattice
   self._scale = ed.scale and ed.scale or 0.1
   if ed.origin then
      self._origin = Point3f(ed.origin.x, ed.origin.y, ed.origin.z)
   else
      self._origin = Point3f(0, 0, 0)
   end

   -- Call _update_shape whenever the collision shape changes
   self._vertical_pathing_region = self._entity:get_component('vertical_pathing_region')
   self._construction_data = self._entity:get_component('stonehearth:construction_data')
   
   self._render_tracker = ConstructionRenderTracker(self._entity)
                              :set_render_region_changed_cb(function(region, visible, view_mode)
                                    self._clip_region = region
                                    self._view_mode = view_mode
                                    self:_update_shape()
                                    h3dSetNodeFlags(self._node, visible and 0 or H3DNodeFlags.Inactive, false)
                                 end)
                              :set_visible_changed_cb(function(visible)
                                    h3dSetNodeFlags(self._node, visible and 0 or H3DNodeFlags.Inactive, false)
                                 end)

   self._cd_promise = self._construction_data:trace_data('drawing ladder', TraceCategories.SYNC_TRACE)
                                          :on_changed(function ()
                                                self._render_tracker:set_normal(self._construction_data:get_normal())
                                             end)
                                          :push_object_state()

   self._vpr_promise = self._vertical_pathing_region:trace_region('drawing ladder', TraceCategories.SYNC_TRACE)
                                          :on_changed(function ()
                                                self._render_tracker:set_region(self._vertical_pathing_region:get_region())
                                             end)
                                          :push_object_state()

   self._ladder_promise = self._ladder:trace('drawing ladder', TraceCategories.SYNC_TRACE)
                                          :on_changed(function ()
                                                self:_update_shape()
                                             end)
                                          :push_object_state()
end

function LadderRenderer:destroy()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil   
   end
   if self._vpr_promise then
      self._vpr_promise:destroy()
      self._vpr_promise = nil
   end
   if self._cd_promise then
      self._cd_promise:destroy()
      self._cd_promise = nil
   end
end

function LadderRenderer:_update_shape()
   local clip_region
   local ladder_region = self._vertical_pathing_region:get_region():get()

   -- update the normal
   local normal = self._construction_data:get_normal()
   if normal then
      self._rotation = voxel_brush_util.normal_to_rotation(normal)
   end

   -- update the ladder
   local ladder_bounds = ladder_region:get_bounds()   
   assert(ladder_bounds.min == Point3.zero)

   local ladder_height = math.max(ladder_bounds.max.y, self._ladder:get_desired_height())
   local ladder_points, ghost_points = {}, {}
   for y= 0, ladder_height-1 do
      local pt = Point3(0, y, 0)
      -- ignore this for now
      -- if not self._clip_region or self._clip_region:get():contains(pt) then 
      if ladder_region:contains(pt) then
         table.insert(ladder_points, pt)
      else
         table.insert(ghost_points, pt)
      end
   end

   self:_rebuild_ladder_nodes(ladder_points, ghost_points)
end

function LadderRenderer:_move_node(node, pt) 
   local offset = self._origin:scaled(.1) + pt:to_float()
   h3dSetNodeTransform(node:get_node(), offset.x, offset.y, offset.z, 0, self._rotation, 0, self._scale, self._scale, self._scale)
end

function LadderRenderer:_create_node(pt, matrix) 
   local node = _radiant.client.create_qubicle_matrix_node(self._node, self._lattice, 'ladder', self._origin)
   self:_move_node(node, pt)
   return node
end

function LadderRenderer:_rebuild_ladder_nodes(points, ghost_points)
   -- make sure we have enough nodes!
   local num_points = #points + #ghost_points
   local needed = num_points - #self._ladder_nodes
   for _ = 1,needed do
      table.insert(self._ladder_nodes, self:_create_node(Point3(0, 0, 0), 'ladder'))
   end

   local ladder_size = #self._ladder_nodes
   local i = 1
   for _, pt in ipairs(points) do
      local node = self._ladder_nodes[i]
      self:_move_node(node, pt)    
      node:set_material('materials/voxel.material.xml')
      node:set_visible(true)
      i = i + 1
   end

   for _, pt in ipairs(ghost_points) do
      local node = self._ladder_nodes[i]
      self:_move_node(node, pt)    
      node:set_material('materials/ghost_item.xml')
      node:set_visible(true)
      i = i + 1
   end

   while i <= ladder_size do
      self._ladder_nodes[i]:set_visible(false)
      i = i + 1
   end
end

return LadderRenderer

