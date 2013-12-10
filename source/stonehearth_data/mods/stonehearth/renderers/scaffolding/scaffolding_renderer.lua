local Point3f = _radiant.csg.Point3f
local Region3 = _radiant.csg.Region3

local ScaffoldingRenderer = class()

-- A lookup table to convert a normal in the xz-plane to a rotation
-- about the y-axis.  Usage: ROTATION_TABLE[normal.x][normal.z]
local ROTATION_TABLE = {
   [ 0] = {
      [-1] = 0,
      [ 1] = 180,
   },
   [ 1] = {
      [ 0] = 270
   },
   [-1] = {
      [ 0] = 90
   }   
}
function ScaffoldingRenderer:__init(render_entity, ed)
   -- Pull some render parameters out of the entity data
   self._lattice = ed.lattice
   self._scale = ed.scale and ed.scale or 0.1
   if ed.origin then
      self._origin = Point3f(ed.origin.x, ed.origin.y, ed.origin.z)
   else
      self._origin = Point3f(0, 0, 0)
   end

   self._pattern_width = ed.width
   self._pattern_height = ed.height
   
   -- Create a new group node to store the individual nodes of each
   -- segment of the lattice.
   self._node = h3dAddGroupNode(render_entity:get_node(), 'scaffolding node')
   
   -- The segments table is a 3d-array containing each segment of the lattice,
   -- indexed by the y, x, z coordinates of each segment.  The segment_region
   -- is a Region3() representing the area of all the current segments.  This
   -- is used to speed up iteration (see _update_shape())
   self._segments = {}
   self._segment_region = Region3()

   -- Call _update_shape whenever the collision shape changes
   self._entity = render_entity:get_entity()
   self._collsion_shape = self._entity:add_component('region_collision_shape')
   if self._collsion_shape then
      self._promise = self._collsion_shape:trace_region('drawing scaffolding')
                                             :on_changed(function ()
                                                self:_update_shape()
                                             end)
      self:_update_shape()
   end
end

function ScaffoldingRenderer:destroy()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
end

function ScaffoldingRenderer:_update_shape()
   local missing = Region3()
   local extra = Region3()

   -- Compute 2 regions: 'missing' contains the points which are missing from the
   -- current render view of the scaffolding (i.e. those that are in our collision
   -- shape, but not yet rendered).  'extra' contains the opposite: stuff we need
   -- to get rid of.
   local region = self._collsion_shape:get_region():get()
   if region then
      missing = region - self._segment_region
      extra = self._segment_region - region
      
      self._segment_region:copy_region(region)
   else
      extra:copy_region(self._segment_region)
      self._segment_region:clear()
   end

   -- Compute the y-rotation for all the nodes.  This is based on the direction of
   -- the scaffolding normal contained in the stonehearth:construction_data component.
   self._rotation = 0
   local construction_data = self._entity:get_component_data('stonehearth:construction_data')
   if construction_data then
      local normal = construction_data.normal
      if normal then
         self._rotation = ROTATION_TABLE[normal.x][normal.z]
         self._tangent = normal.x == 0 and 'x' or 'z'
      end
   end
   
   -- For every point that's missing, add a new piece of scaffolding.
   for cube in missing:each_cube() do
      for pt in cube:each_point() do
         local node = self:_create_segment_node(pt)
         self:_add_segment(pt, node)
      end
   end

   -- For every point that's extra, remove that bit of scaffolding
   for cube in extra:each_cube() do
      for pt in cube:each_point() do
         self:_remove_segment(pt)
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

   --Derive name of matrix (part of lattice we need from pt)
   local matrix = string.format('scaffold_%d_%d',x, y )
   
   --need to flip 180 around y axis
   self._rotation = (self._rotation  + 180) % 360

   local node = _radiant.client.create_qubicle_matrix_node(self._node, self._lattice, matrix, self._origin)
   h3dSetNodeTransform(node, pt.x, pt.y, pt.z, 0, self._rotation, 0, self._scale, self._scale, self._scale)
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
               h3dRemoveNode(segment.node)
            end
            row[pt.z] = nil
         end
      end
   end
end

return ScaffoldingRenderer

