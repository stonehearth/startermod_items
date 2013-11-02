local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyPortal = require 'services.build.proxy_portal'
local ProxyWall = class(ProxyFabrication)
local Constants = require 'services.build.constants'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyWall:__init(parent_proxy, arg1)
   self[ProxyFabrication]:__init(self, parent_proxy, arg1)
end

function ProxyWall:_get_tangent_and_normal_coords()
   local normal = self:get_normal()
   assert(normal:distance_squared() == 1)
   
   if normal.x == 0 then
      return 'x', 'z'
   end
   return 'z', 'x'
end

function ProxyWall:layout(brush) 
   -- create a region that covers the span of the wall and paint the shape
   -- of the brush into it, which will clip out all the transparent parts
   if not brush then
      brush = self:get_voxel_brush()
   end
   local bounds = Region3(Cube3(self._start_pt, self._end_pt, 0))
   local collsion_shape = brush:paint_through_stencil(bounds)
   self:get_region():modify():copy_region(collsion_shape)

   -- adjust the shape based on our children   
   for _, proxy in pairs(self:get_children()) do
      self:_rotate_proxy(proxy)
      if proxy:is_a(ProxyPortal) then
         self:_stencil_portal(proxy)
      end
   end
end

function ProxyWall:_rotate_proxy(proxy)
   proxy:turn_to(self._rotation)
end

function ProxyWall:_stencil_portal(portal)
   -- cut the portal all the way through the wall.
   local cutter2 = portal:get_cutter()
   local cutter3 = Region3()
   local t, n = self:_get_tangent_and_normal_coords()
   
   local origin = portal:get_location()
   for r2 in cutter2:contents() do
      local min = Point3(0, r2.min.y + origin.y, 0)
      local max = Point3(0, r2.max.y + origin.y, 0)
      min[t] = r2.min.x + origin[t]
      max[t] = r2.max.x + origin[t]
      min[n] = self._start_pt[n]
      max[n] = self._end_pt[n]
      cutter3:add_unique_cube(Cube3(min, max))
   end
   self:get_region():modify():subtract_region(cutter3)
end

function ProxyWall:get_length()
   local t, n = self:_get_tangent_and_normal_coords()
   return self._end_pt[t] - self._start_pt[t]
end

function ProxyWall:reface_point(pt)
   local t, n = self:_get_tangent_and_normal_coords()
   local offset = Point3(0, 0, 0)
   offset[t] = self._start_pt[t] + pt.x
   offset.y  = self._start_pt.y  + pt.y
   offset[n] = self._start_pt[n] + pt.z
   return offset
end

function ProxyWall:connect_to(column_a, column_b)
   local pos_a = column_a:get_location()
   local pos_b = column_b:get_location()
   self:connect_to_points(pos_a, pos_b)
   column_a:connect_to_wall(self)
   column_b:connect_to_wall(self)
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
   local span, rotation
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
   local rotations = {
      [-1] = { [-1] = 270, [1] = 180 },
      [ 1] = { [-1] = 0,   [1] = 90 },
   }
   self._rotation = rotations[tangent[t]][normal[n]]   
   
   self:set_normal(normal)
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
   local brush = self:get_voxel_brush()
   local model = brush:paint_once()
   local bounds = model:get_bounds()
   end_pt[n] = bounds.max[n]
   start_pt[n] = bounds.min[n]
   
   self._start_pt = start_pt
   self._end_pt   = end_pt
   
   self:layout(brush) -- should this only happen on-demand?
end

return ProxyWall
