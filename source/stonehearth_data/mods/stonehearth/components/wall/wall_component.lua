local constants = require('constants').construction

local Wall = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local INFINITE = 10000000

-- called to initialize the component on creation and loading.
--
function Wall:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.initialized then
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   end
end

function Wall:begin_editing(other_wall)
   self._sv.start_pt = Point3(other_wall._sv.start_pt)
   self._sv.end_pt = Point3(other_wall._sv.end_pt)
   self.__saved_variables:mark_changed()
   return self
end

-- make the destination region match the shape of the column.  layout
-- should be called sometime after doing something which could change
-- the wall shape, but before anyone else depends on that shape.  to
-- be safe, just call it immediately afterwards, but sometimes batching
-- is useful (e.g. connect_to(), attach_to_roof())
-- 
function Wall:layout()
   local building = self._entity:add_component('mob'):get_parent()

   local start_pt, end_pt = self._sv.start_pt, self._sv.end_pt

   local t = (math.abs(start_pt.x - end_pt.x) == 1) and 'z' or 'x'
   local n = t == 'x' and 'z' or 'x'

   if start_pt[t] == end_pt[t] then
      return
   end
   assert(start_pt.x < end_pt.x)
   assert(start_pt.y < end_pt.y)
   assert(start_pt.z < end_pt.z)

   --[[
   -- DO NOT DELETE THIS CODE!  I will use it soon.

   -- omfg... righthandedness is screwing with my brain.
   local rotations = {
      [-1] = { [-1] = 270, [1] = 180 },
      [ 1] = { [-1] = 0,   [1] = 90 },
   }
   self._rotation = rotations[tangent[t] ][normal[n] ]
   ]]
   
   -- if there are no structure at all overlapping the region we can create the
   -- wall and add it to the building.  otherwise, don't.
   local origin = radiant.entities.get_world_grid_location(self._entity)
   local world_bounds = Cube3(start_pt, end_pt):translated(origin)
   local overlapping = radiant.terrain.get_entities_in_cube(world_bounds, function(entity)
         return stonehearth.build:is_blueprint(entity)
      end)
      
   overlapping[self._entity:get_id()]  = nil
   if next(overlapping) ~= nil then
      --radiant.entities.destroy_entity(wall)
      return
   end

   -- paint again through a stencil covering the entire span of the
   -- wall to compute the wall shape
   local stencil = self:_compute_wall_shape()
   local collsion_shape = self._entity:get_component('stonehearth:construction_data')
                                        :create_voxel_brush()
                                        :paint_through_stencil(stencil)
   self._entity:add_component('destination')
                  :get_region()
                  :modify(function(cursor)
                        cursor:copy_region(collsion_shape)
                     end)
                     
   return self
end

-- reshapes the wall to connect to `column_a` and `column_b` pointing in the direction
-- of `normal`.  this automatically orients the wall in the correct direction, meaning
-- the actual wall position will either be near `column_a` or `column_b`, depending.
--
--    @param column_a - one of the columns to attach the wall to
--    @param column_b - the other column to attach the wall to
--    @param normal - a Point3 pointing "outside" of the wall
-- 
function Wall:connect_to(column_a, column_b, normal)
   local building = self._entity:get_component('mob'):get_parent()

   self._entity:get_component('stonehearth:construction_data')
               :set_normal(normal)

   local pos_a = radiant.entities.get_location_aligned(column_a)
   local pos_b = radiant.entities.get_location_aligned(column_b)
   local origin = radiant.entities.get_world_grid_location(building)

   -- figure out the tangent and normal coordinate indicies based on the direction
   -- of the normal
   local t, n
   if pos_a.x == pos_b.x then
      t, n = 'z', 'x'
   else
      t, n = 'x', 'z'
   end   
   assert(pos_a[n] == pos_b[n], 'points are not co-linear')
   assert(pos_a[t] <  pos_b[t], 'points are not sorted')

   -- the origin of the wall will be at the start or the end, depending on the
   -- normal.  to make the tiling of the voxels of the wall look good, ensure
   -- that the 0,0 voxel is never adjoining a shared column of *both* walls
   -- connected to that column.  otherwise, you get this weird mirror effect.
   local flip
   if normal.x ~= 0 then
      flip = normal.x < 0
   elseif normal.z ~= 0 then
      flip = normal.z > 0
   end
   if flip then
      pos_a, pos_b = pos_b, pos_a
   end

   -- compoute the width of the wall and a tangent vector which will march
   -- us from pos_a to pos_b.
   local tangent = Point3(0, 0, 0)
   self._sv.start_pt = Point3(0, 0, 0)
   self._sv.end_pt = Point3(1, constants.STOREY_HEIGHT, 1)

   if pos_a[t] < pos_b[t] then
      tangent[t] = 1
      self._sv.end_pt[t] = pos_b[t] - pos_a[t] - 1
   else
      tangent[t] = -1
      self._sv.start_pt[t] = -(pos_a[t] - pos_b[t] - 2)
   end

   -- paint once to get the depth of the wall
   local brush_bounds = self._entity:get_component('stonehearth:construction_data')
                                       :create_voxel_brush()
                                       :paint_once()
                                       :get_bounds()
   self._sv.end_pt[n] = brush_bounds.max[n]
   self._sv.start_pt[n] = brush_bounds.min[n]

   radiant.entities.move_to(self._entity, pos_a + tangent)

   self.__saved_variables:mark_changed()
   
   return self
end

-- connect the wall to the roof.  the next time you call layout, we'll make
-- sure to grow the wall height all the way up to the roof
--
--    @param roof - the roof to attach to
--
function Wall:connect_to_roof(roof)
   self._sv.roof = roof
   self.__saved_variables:mark_changed()
   return self
end

-- computes the shape of the wall.  the wall goes from start_pt to end_pt
-- in the direction of normal.  it is also constants.STOREY_HEIGHT high,
-- unless it has a roof attached, in which case it extends all the way to
-- the bottom of the roof.
--
function Wall:_compute_wall_shape()
   local p0, p1 = self._sv.start_pt, self._sv.end_pt

   local shape = Region3(Cube3(p0, p1, 0))

   -- if there's a roof over our heads, make sure the wall extends all
   -- the way up to it.
   if self._sv.roof then
      local roof = self._sv.roof
      local roof_region = roof:get_component('destination'):get_region():get()

      local stencil = Cube3(Point3(p0.x, p1.y, p0.z),
                            Point3(p1.x, INFINITE, p1.z))
      
      -- translate the stencil into the roof's coordinate system, clip it,
      -- then transform the result back to our coordinate system
      local offset = radiant.entities.get_location_aligned(roof) -
                     radiant.entities.get_location_aligned(self._entity)

      local roof_overhang = roof_region:clipped(stencil:translated(-offset))                                       
                                       :translated(offset)
      
      -- iterate through each "shingle" in the overhang, growing the wall
      -- upwards toward the base of the shingle.
      for shingle in roof_overhang:each_cube() do
         local col = Cube3(Point3(shingle.min.x, p1.y, shingle.min.z),
                           Point3(shingle.max.x, shingle.min.y, shingle.max.z))
         shape:add_unique_cube(col)
      end
   end

   return shape
end

return Wall
