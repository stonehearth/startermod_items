local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyWall = class(ProxyFabrication)
local Constants = require 'services.build.constants'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyWall:__init(arg1)
   self[ProxyFabrication]:__init(arg1, 'stonehearth:wall')   
end

function ProxyWall:connect_to(column_a, column_b)
   local pos_a = column_a:get_location()
   local pos_b = column_b:get_location()
   self:connect_to_points(pos_a, pos_b)
   return self
end

function ProxyWall:connect_to_points(pos_a, pos_b)
   local t, n
   if pos_a.x == pos_b.x then
      t, n = 'z', 'x'
   else
      t, n = 'x', 'z'
   end   
   assert(pos_a[n] == pos_b[n], 'points are not co-linear')
      
   -- we draw the wall from start-to-end.  if the start point is less than the
   -- end, the normal points in the positive direction.  otherwise, negative
   local span
   local normal = Point3(0, 0, 0)   
   local tangent = Point3(0, 0, 0)

   if pos_a[t] < pos_b[t] then
      tangent[t] = 1
      span = pos_b[t] - pos_a[t]
   else
      tangent[t] = -1
      span = pos_a[t] - pos_b[t]
   end
   -- omfg... righthandedness is screwing with my brain.
   normal[n] = t == 'x' and -tangent[t] or tangent[t]

   self:get_entity():add_component('mob'):set_location_grid_aligned(pos_a + tangent)
   
   local start_pt = Point3(0, 0, 0)
   local end_pt = Point3(1, Constants.STOREY_HEIGHT, 1) -- that "1" should be the depth of the wall.
   if tangent[t] < 0 then
      start_pt[t] = -(span - 2)
   else
      end_pt[t] = span - 1
   end
   assert(start_pt.x < end_pt.x)
   assert(start_pt.y < end_pt.y)
   assert(start_pt.z < end_pt.z)

  
   -- grow the bounds by the depth of our cursor...
   local fabinfo = radiant.entities.get_entity_data(self:get_entity(), 'stonehearth:voxel_brush_info')
   assert(fabinfo, 'wall is missing stonehearth:voxel_brush_info invariant')
   
   local brush = _radiant.voxel.create_brush(fabinfo.brush)
   local model = brush:set_normal(normal)
                      :set_paint_mode(_radiant.voxel.QubicleBrush.Opaque)
                      :paint()
   local bounds = model:get_bounds()
   end_pt[n] = bounds.max[n]
   start_pt[n] = bounds.min[n]
   
   -- create a region that covers the span of the wall and paint the shape
   -- of the brush into it, which will clip out all the transparent parts
   local bounds = Region3(Cube3(start_pt, end_pt, 0))   
   local collsion_shape = brush:paint_through_stencil(bounds)
   
   -- finally, set that to be the collsion shape for the wall
   self:get_region():modify():copy_region(collsion_shape)
   self:set_normal(normal)
   self:set_tangent(tangent)
end

return ProxyWall
