local voxel_brush_util = require 'services.server.build.voxel_brush_util'
local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Point3f = _radiant.csg.Point3f
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local ScaffoldingRenderer = class()
local log = radiant.log.create_logger('scaffolding.renderer')

function ScaffoldingRenderer:__init(render_entity, ed)
   -- Pull some render parameters out of the entity data
   self._lattice = ed.lattice
   self._scale = ed.scale and ed.scale or 0.1
   if ed.origin then
      self._origin = Point3f(ed.origin.x, ed.origin.y, ed.origin.z)
   else
      self._origin = Point3f(0, 0, 0)
   end

   self._entity_node = render_entity:get_node()
   self._pattern_width = ed.width
   self._pattern_height = ed.height
   
   -- Create a new group node to store the individual nodes of each
   -- segment of the lattice.
   self._node = h3dAddGroupNode(self._entity_node, 'scaffolding node')
   
   -- The segments table is a 3d-array containing each segment of the lattice,
   -- indexed by the y, x, z coordinates of each segment.  The segment_region
   -- is a Region3() representing the area of all the current segments.  This
   -- is used to speed up iteration (see _update_shape())
   self._segments = {}
   self._tops = {}
   self._segment_region = Region3()

   -- Call _update_shape whenever the collision shape changes
   self._entity = render_entity:get_entity()
   self._collsion_shape = self._entity:add_component('region_collision_shape')
   self._construction_data = self._entity:get_component('stonehearth:construction_data')
   
   if self._collsion_shape and self._construction_data then
      -- create a new ConstructionRenderTracker to track the hud mode and build
      -- view mode.  it will drive the shape and visibility of our structure shape
      -- based on those modes.  
      self._render_tracker = ConstructionRenderTracker(self._entity)
                                 :set_normal(self._construction_data:get_normal())
                                 :set_render_region_changed_cb(function(region, visible, view_mode)
                                       self._draw_region = region
                                       self:_update_shape(view_mode)
                                       h3dSetNodeFlags(self._node, visible and 0 or H3DNodeFlags.Inactive, false)
                                    end)
                                 :set_visible_changed_cb(function(visible)
                                       h3dSetNodeFlags(self._node, visible and 0 or H3DNodeFlags.Inactive, false)
                                    end)

      -- push the collision shape of the scaffolding into the render tracker
      -- whenever it changes
      self._promise = self._collsion_shape:trace_region('drawing scaffolding')
                                             :on_changed(function ()
                                                   self._render_tracker:set_region(self._collsion_shape:get_region())
                                                end)
                                             :push_object_state()
   end

end

function ScaffoldingRenderer:destroy()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil   
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
end

function ScaffoldingRenderer:_update_shape(mode)
   local missing = Region3()
   local extra = Region3()
   local show_region, hide_region

   -- Compute 2 regions: 'missing' contains the points which are missing from the
   -- current render view of the scaffolding (i.e. those that are in our collision
   -- shape, but not yet rendered).  'extra' contains the opposite: stuff we need
   -- to get rid of.
   local solid_region = self._collsion_shape:get_region():get()
   if solid_region then
      missing = solid_region - self._segment_region
      extra = self._segment_region - solid_region
      
      self._segment_region:copy_region(solid_region)

      -- in rpg mode, hide all the nodes which do not directly overlap
      -- the draw region (set by the render_tracker)
      if mode == 'rpg' then
         if self._draw_region then
            show_region = self._draw_region:get():intersected(solid_region)
            hide_region = solid_region - show_region
         else
            hide_region = solid_region
         end
      end
   else
      extra:copy_region(self._segment_region)
      self._segment_region:clear()
   end


   -- Compute the y-rotation for all the nodes.  This is based on the direction of
   -- the scaffolding normal contained in the stonehearth:construction_data component.
   self._tangent = 'x'
   self._rotation = 0
   if self._construction_data then
      local normal = self._construction_data:get_normal()
      if normal then
         self._rotation = voxel_brush_util.normal_to_rotation(normal)
         self._tangent = normal.x == 0 and 'x' or 'z'
      end
   end
   
   -- For every point that's missing, add a new piece of scaffolding.
   for cube in missing:each_cube() do
      for pt in cube:each_point() do
         local node = self:_create_segment_node(pt)
         self:_add_segment(pt, node)
         self:_move_top(pt)
      end
   end

   -- For every point that's extra, remove that bit of scaffolding
   for cube in extra:each_cube() do
      for pt in cube:each_point() do
         self:_remove_segment(pt)
         self:_move_top(pt)
      end
   end

   if mode == 'rpg' then
      -- show all the segments inside the draw region
      if show_region then
         for cube in show_region:each_cube() do
            for pt in cube:each_point() do
               self:_get_segment_node(pt)
                        :set_visible(true)
            end
         end
      end
      -- hide all the segments outside the draw region
      if hide_region then
         for cube in hide_region:each_cube() do
            for pt in cube:each_point() do
               self:_get_segment_node(pt)
                        :set_visible(false)
            end
         end
      end
      -- show or hide the tops, too.
      for x, t in ipairs(self._tops) do
         for z, top in ipairs(t) do
            local pt = Point3(x, top.top_y, z)
            -- i have *no idea* why this doesn't work. =(
            top.node:set_visible(show_region and show_region:contains(pt))
         end
      end
   end
end

--- Return a hord3d node for the scaffolding at point.
--  Make sure we scale and rotate it correctly,
--  and position it where it belongs relative to the scaffolding origin.
function ScaffoldingRenderer:_create_segment_node(pt) 
   --Use the direction we're facing for the X coordinate of the lattice node we want
   --Layout from left to right
   local tangent = math.abs(pt[self._tangent])    
   local x = (self._pattern_width - 1) - tangent % self._pattern_width
   local y = pt.y % self._pattern_height
   --TODO: derive z; right now our scaffolding is always z=0
   local z = 0

   --Derive name of matrix (part of lattice we need from pt)
   local matrix = string.format('scaffold_%d_%d_%d',x, y, z)
   return self:_create_node(pt, matrix)
end

function ScaffoldingRenderer:_create_node(pt, matrix) 
   local offset = self._origin:scaled(.1) + pt:to_float()
   local node = _radiant.client.create_qubicle_matrix_node(self._node, self._lattice, matrix, self._origin)
   h3dSetNodeTransform(node:get_node(), offset.x, offset.y, offset.z, 0, self._rotation, 0, self._scale, self._scale, self._scale)
   --h3dSetNodeFlags(node:get_node(), h3dGetNodeFlags(self._entity_node), true);
   return node
end

function ScaffoldingRenderer:_add_segment(pt, node)
   -- Add a segment from the segments table
   if not self._segments[pt.y] then
      self._segments[pt.y] = {}
   end
   if not self._segments[pt.y][pt.x] then
      self._segments[pt.y][pt.x] = {}
   end
   self._segments[pt.y][pt.x][pt.z] = {
      node = node,
   }
end

function ScaffoldingRenderer:_remove_segment(pt)
   -- Remove a segment from the segments table
   local matrix = self._segments[pt.y]
   if matrix then
      local row = matrix[pt.x]
      if row then
         local segment = row[pt.z]
         if segment then
            if segment.node then
               segment.node:destroy()
            end
            row[pt.z] = nil
         end
      end
   end
end

function ScaffoldingRenderer:_get_segment_node(pt)
   -- Remove a segment from the segments table
   local matrix = self._segments[pt.y]
   if matrix then
      local row = matrix[pt.x]
      if row then
         local segment = row[pt.z]
         if segment then
            return segment.node
         end
      end
   end
end

--- When a piece of scaffolding is added/removed, move the top segment to the correct spot.
--  @param pt - the point the top should be moved to. 
function ScaffoldingRenderer:_move_top(pt)
   local top_data = self:_get_top_node(pt)
   local old_top_y = top_data.top_y
   local new_top_y = self:_get_highest_y(pt)
   if new_top_y ~= old_top_y then
      --If the top y has changed...   
      if new_top_y <= -1 then
         --If the top y is -1, then we should remove the node
         top_data.node:destroy()
         self._tops[pt.x][pt.z] = nil
      else
         --Otherwise, just move the top
         local offset = self._origin:scaled(.1) + pt:to_float()
         h3dSetNodeTransform(top_data.node:get_node(), offset.x, new_top_y, offset.z, 0, self._rotation, 0, self._scale, self._scale, self._scale)
      end
   end
end

function ScaffoldingRenderer:_get_top_node(pt)
   if not self._tops[pt.x] then
      self._tops[pt.x] = {}
   end
   if not self._tops[pt.x][pt.z] then
      self._tops[pt.x][pt.z] = {}
      
      local tangent = math.abs(pt[self._tangent])    
      local x = (self._pattern_width - 1) - tangent % self._pattern_width
      --TODO: derive z; right now our scaffolding is always z=0
      local z = 0
      local matrix = string.format('scaffold_%d_top_%d', x, z)
      
      self._tops[pt.x][pt.z] = {
         node = self:_create_node(pt, matrix), 
         top_y = pt.y
      }
   end
   return self._tops[pt.x][pt.z]
end

function ScaffoldingRenderer:_get_highest_y(pt)
   local scaffold_node = self._segments[pt.y][pt.x][pt.z]
   local top_node_data = self:_get_top_node(pt)

   if scaffold_node then
      --This pt was just added
      if pt.y > top_node_data.top_y then
         top_node_data.top_y = pt.y
      end
   else
      --this pt was just removed. Iterate through the remaining and find the highest y
      local highest_y = pt.y - 1
      for i, x_z_matrix in ipairs(self._segments) do
         if i > highest_y and x_z_matrix[pt.x] and x_z_matrix[pt.x][pt.z] then
            highest_y = i
         end
      end
      top_node_data.top_y = highest_y
   end
   return top_node_data.top_y
end


return ScaffoldingRenderer

