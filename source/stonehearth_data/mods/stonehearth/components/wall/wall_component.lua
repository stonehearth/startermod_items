local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Wall = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

-- enumerate the range (-2, -2) - (2, 2) in the order which is most
-- likely the least visually disturbing to the user
local TWEAK_OFFSETS = {
   Point2( 1,  0),
   Point2(-1,  0),
   Point2( 0, -1),
   Point2( 0,  1),

   Point2( 1, -1),
   Point2(-1, -1),
   Point2( 1,  1),
   Point2(-1,  1),

   Point2( 2,  0),
   Point2(-2,  0),
   Point2( 2, -1),
   Point2(-2, -1),
   Point2( 2,  1),
   Point2(-2,  1),

   Point2( 0, -2),
   Point2( 0,  2),
   Point2( 1, -2),
   Point2( 1,  2),
   Point2(-1, -2),
   Point2(-1,  2),
   Point2( 2, -2),
   Point2( 2,  2),
   Point2(-2, -2),
   Point2(-2,  2),
}

-- called to initialize the component on creation and loading.
--
function Wall:initialize(entity, json)
   assert(entity and entity:is_valid())
   
   self._entity = entity

   self._log = radiant.log.create_logger('build.wall')
                              :set_entity(entity)

   if not self._sv.initialized then
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      self:_compute_wall_measurements()
   end   
end

function Wall:set_brush(brush)   
   self._sv.brush = brush
   self.__saved_variables:mark_changed()
   self._log:detail('set brush (brush %s)', self._sv.brush)
   return self
end

function Wall:get_brush()
   return self._sv.brush
end


-- returns whether or not this wall is a patch wall.  patch walls are inserted
-- by the roofing process to plug holes near roof junctions.
--
function Wall:is_patch_wall()
   return self._sv.patch_wall_region ~= nil
end

-- initializes the current wall as a patch wall.  patch walls are inserted
-- by the roofing process to plug holes near roof junctions.  they're not
-- connected to columns like other walls, but otherwise behave similarly.
--
--    @param normal - the wall normal
--    @param region - a Region3 containing the exact shape of the patch wall
--
function Wall:create_patch_wall(normal, region)
   local bounds = region:get_bounds()
   self._sv.patch_wall_region = region
   self._sv.normal = normal
   self.__saved_variables:mark_changed()

   self._start_pt = bounds.min
   self._end_pt = bounds.max

   -- forward the normal over to our construction_data component.  
   self._entity:add_component('stonehearth:construction_data')
                  :set_normal(normal)
   return self
end

function Wall:clone_from(entity)
   if entity then
      local other_wall = entity:get_component('stonehearth:wall')

      self._sv.pos_a = other_wall._sv.pos_a
      self._sv.pos_b = other_wall._sv.pos_b
      self._sv.normal = other_wall._sv.normal
      self._sv.brush = other_wall._sv.brush
      self._sv.patch_wall_region = other_wall._sv.patch_wall_region
      self.__saved_variables:mark_changed()
      self:_compute_wall_measurements()
   end
   return self
end

function Wall:begin_editing(entity)
   self._editing = true
   self._editing_region = entity:get_component('destination'):get_region()

   return self
end

function Wall:get_editing_reserved_region()
   return self._editing_region
end

function Wall:get_tangent_coord()
   return self._tangent_coord
end

function Wall:get_normal_coord()
   return self._normal_coord
end

function Wall:get_normal()
   return self._sv.normal
end

function Wall:compute_fixture_placement(fixture_entity, location)
   local t, n = self._tangent_coord, self._normal_coord
   local start_pt, end_pt = self._start_pt, self._end_pt

   -- if there's no fixture component, it cannot be placed on a wall.
   local fixture = fixture_entity:get_component('stonehearth:fixture')
   if not fixture then
      return nil
   end

   -- if there's a portal component, make sure the fixture goes in the
   -- wall.  otherwise, it must be 1 removed in the normal direction.
   local portal = fixture_entity:get_component('stonehearth:portal')
   if portal then
      location[n] = 0
   else
      location[n] = self._sv.normal[n]
   end

   -- fix alignment issues
   local valign = fixture:get_valign()
   local bounds = fixture:get_bounds()
   local margin = fixture:get_margin()

   if valign == "bottom" then
      location.y = start_pt.y + margin.bottom - bounds.min.y
   end

   -- make sure the fixture fits within its margin constraints
   local box = Cube3()
   box.max.y = location.y + bounds.max.y + margin.top
   box.min.y = location.y + bounds.min.y - margin.bottom   
   box.min[t] = location[t] + bounds.min.x - margin.left
   box.max[t] = location[t] + bounds.max.x + margin.right
   box.min[n] = start_pt[n]
   box.max[n] = end_pt[n]
   box = Region3(box)
   
   local shape = self._editing_region:get()
   local overhang = box - shape
   
   if overhang:empty() then
      return location
   end

   -- try to nudge it over.  if that doesn't work, bail.
   for _, tweak in ipairs(TWEAK_OFFSETS) do
      local offset = Point3(0, tweak.y, 0)
      offset[t] = tweak.x
      if (box:translated(offset) - shape):empty() then
         return location:translated(offset)
      end
   end
   
   -- no luck!
   return nil
end

function Wall:_get_portal_region(portal_entity, portal)
   local t, n = self._tangent_coord, self._normal_coord
   local start_pt, end_pt = self._start_pt, self._end_pt
   local origin = portal_entity:get_component('mob'):get_grid_location()

   local region2 = portal:get_portal_region()
   local region3 = Region3()
   for r2 in region2:each_cube() do
      local min = Point3(0, r2.min.y, 0)
      local max = Point3(0, r2.max.y, 0)

      min[t] = r2.min.x
      max[t] = r2.max.x
      min[n] = start_pt[n]
      max[n] = end_pt[n]

      local cube = Cube3(min, max)
      region3:add_unique_cube(cube)
   end
   return region3:translated(origin)
end


function Wall:add_fixture(fixture, location, normal)
   if not normal then
      -- if no normal specified, use the normal of the wall
      normal = self._sv.normal
   end

   local rotation = build_util.normal_to_rotation(normal)
   radiant.entities.add_child(self._entity, fixture, location)
   fixture:get_component('mob'):turn_to(rotation)
   
   return self
end

function Wall:remove_fixture(fixture)
   radiant.entities.remove_child(self._entity, fixture)
   return self
end


-- make the destination region match the shape of the column.  layout
-- should be called sometime after doing something which could change
-- the wall shape, but before anyone else depends on that shape.  to
-- be safe, just call it immediately afterwards, but sometimes batching
-- is useful (e.g. connect_to(), attach_to_roof())
-- 
function Wall:layout()
   local building = build_util.get_building_for(self._entity)
   if not building then
      -- sometimes, depending on the order that things get destroyed, a wall
      -- will be asked to layout after it has been divorces from it's building
      -- (e.g. when the blueprint still exists, but the project (and thus the
      -- fabricator) has been destroyed).
      return
   end

   local function compute_collision_shape()
      local stencil = self:_compute_wall_shape(building)
      return self._entity:get_component('stonehearth:construction_progress')
                               :create_voxel_brush(self._sv.brush)
                               :paint_through_stencil(stencil)
   end

   local collision_shape
   if not self._editing then
      -- server side...
      collision_shape = compute_collision_shape()
   else
      -- client side...
      if not self._editing_region then
         self._editing_region = _radiant.client.alloc_region3()
         self._editing_region:modify(function(cursor)
               cursor:copy_region(compute_collision_shape())
            end)
      end
      collision_shape = Region3()
      collision_shape:copy_region(self._editing_region:get())
   end
   
   assert(collision_shape)

   -- stencil out the portals
   local ec = self._entity:get_component('entity_container')
   if ec then
      for _, child in ec:each_child() do
         local portal = child:get_component('stonehearth:portal')
         if portal then
            local region3 = self:_get_portal_region(child, portal)
            collision_shape:subtract_region(region3)
         end
      end      
   end

   self._entity:add_component('destination')
                  :get_region()
                  :modify(function(cursor)
                        cursor:copy_region(collision_shape)
                     end)
                     
   return self
end

function Wall:connect_to_roof(roof)
   assert(not self._sv.roof)
   self._sv.roof = roof
   self.__saved_variables:mark_changed()
end

function Wall:get_roof(roof)
   return self._sv.roof
end

-- reshapes the wall to connect to `column_a` and `column_b` pointing in the direction
-- of `normal`.  this automatically orients the wall in the correct direction, meaning
-- the actual wall position will either be near `column_a` or `column_b`, depending.
--
--    @param column_a - one of the columns to attach the wall to
--    @param column_b - the other column to attach the wall to
--    @param normal - a Point3 pointing "outside" of the wall
-- 
function Wall:connect_to_columns(column_a, column_b, normal)
   self._sv.normal = normal
   self._sv.pos_a = radiant.entities.get_location_aligned(column_a)
   self._sv.pos_b = radiant.entities.get_location_aligned(column_b)
   self._sv.column_a = column_a
   self._sv.column_b = column_b
   self.__saved_variables:mark_changed()

   column_a:get_component('stonehearth:column')
               :connect_to_wall(self._entity)

   column_b:get_component('stonehearth:column')
               :connect_to_wall(self._entity)
               
   self:_compute_wall_measurements()
   radiant.entities.move_to(self._entity, self._position)
   return self
end

function Wall:get_columns()
   return self._sv.column_a, self._sv.column_b
end

function Wall:_compute_wall_measurements()
   self._log:detail('computing measurements')

   if self._sv.patch_wall_region then
      return
   end

   local normal, pos_a, pos_b = self._sv.normal, self._sv.pos_a, self._sv.pos_b
   if not self._sv.normal then
      -- we're the project, not the blueprint.  nothing to compute.
      return
   end

   -- figure out the tangent and normal coordinate indicies based on the direction
   -- of the normal
   local t, n
   if pos_a.x == pos_b.x then
      t, n = 'z', 'x'
   else
      t, n = 'x', 'z'
   end   
   assert(pos_a[n] == pos_b[n], 'points are not co-linear')
   if pos_a[t] >  pos_b[t] then
      pos_a, pos_b = pos_b, pos_a  -- make sure pos_a is < pos_b
      self._sv.pos_a, self._sv.pos_b = self._sv.pos_b, self._sv.pos_a
      self.__saved_variables:mark_changed()
   end

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
   self._tangent = Point3(0, 0, 0)
   self._start_pt = Point3(0, 0, 0)
   self._end_pt = Point3(1, constants.STOREY_HEIGHT, 1)

   if pos_a[t] < pos_b[t] then
      self._tangent[t] = 1
      self._end_pt[t] = pos_b[t] - pos_a[t] - 1
   else
      self._tangent[t] = -1
      self._start_pt[t] = -(pos_a[t] - pos_b[t] - 2)
   end

   -- paint once to get the depth of the wall
   -- in a world where walls can be more than 1-unit thick, we will need to do 
   -- this.  it's really expensive, though, so let's not for now
   --[[
   local brush_bounds = self._entity:get_component('stonehearth:construction_data')
                                       :create_voxel_brush()
                                       :paint_once()
                                       :get_bounds()

   self._end_pt[n] = brush_bounds.max.z
   self._start_pt[n] = brush_bounds.min.z
   ]]
   self._tangent_coord = t
   self._normal_coord = n
   self._position = pos_a + self._tangent
end

-- computes the shape of the wall.  the wall goes from start_pt to end_pt
-- in the direction of normal.  it is also constants.STOREY_HEIGHT high,
-- unless it has a roof attached, in which case it extends all the way to
-- the bottom of the roof.
--
function Wall:_compute_wall_shape(building)
   -- if we're a patch wall, the region was passed in directly at creation
   -- time.  no computation need by done   
   if self._sv.patch_wall_region then
      return self._sv.patch_wall_region
   end

   -- otherwise, grow our box up to the roof level
   local box = Cube3(self._start_pt, self._end_pt, 0)
   if box:get_area() == 0 then
      return Region3()
   end

   local roof = self._sv.roof   
   if roof then
      return build_util.grow_local_box_to_roof(roof, self._entity, box)
   end

   return Region3(box)
end


function Wall:save_to_template()
   return {
      normal = self._sv.normal,
      pos_a = self._sv.pos_a,
      pos_b = self._sv.pos_b,
      brush = self._sv.brush,
      roof = build_util.pack_entity(self._sv.roof),
      column_a = build_util.pack_entity(self._sv.column_a),
      column_b = build_util.pack_entity(self._sv.column_b),
      patch_wall_region = self._sv.patch_wall_region,
   }
end

function Wall:load_from_template(data, options, entity_map)
   self._sv.normal = Point3(data.normal.x, data.normal.y, data.normal.z)
   self._sv.brush = data.brush
   if data.patch_wall_region then
      self._sv.patch_wall_region = Region3()
      self._sv.patch_wall_region:load(data.patch_wall_region)
      self.__saved_variables:mark_changed()
      return
   end
   self._sv.pos_a = Point3(data.pos_a.x, data.pos_a.y, data.pos_a.z)
   self._sv.pos_b = Point3(data.pos_b.x, data.pos_b.y, data.pos_b.z)
   self._sv.roof  = build_util.unpack_entity(data.roof, entity_map)
   self._sv.column_a  = build_util.unpack_entity(data.column_a, entity_map)
   self._sv.column_b  = build_util.unpack_entity(data.column_b, entity_map)

   self.__saved_variables:mark_changed()
   self:_compute_wall_measurements()
end

function Wall:rotate_structure(degrees)
   self._sv.normal = self._sv.normal:rotated(degrees)
   if self._sv.patch_wall_region then
      local cursor = self._sv.patch_wall_region
      local origin = Point3(0.5, 0, 0.5)
      cursor:translate(-origin)
      cursor:rotate(degrees)
      cursor:translate(origin)
      local mob = self._entity:get_component('mob')
      mob:move_to(mob:get_location():rotated(degrees))
      return
   end
   self._sv.pos_a = self._sv.pos_a:rotated(degrees)
   self._sv.pos_b = self._sv.pos_b:rotated(degrees)
   self.__saved_variables:mark_changed()
   self:_compute_wall_measurements()
   radiant.entities.move_to(self._entity, self._position)
   --self:layout() required!
end

return Wall
