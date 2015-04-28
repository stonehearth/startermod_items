local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local csg_lib = {}

csg_lib.XZ_DIRECTIONS = {
   Point3(-1, 0, 0),
   Point3( 1, 0, 0),
   Point3( 0, 0,-1),
   Point3( 0, 0, 1)
}

csg_lib.XYZ_DIRECTIONS = {
   Point3(-1, 0, 0),
   Point3( 1, 0, 0),
   Point3( 0, 0,-1),
   Point3( 0, 0, 1),
   Point3( 0,-1, 0),
   Point3( 0, 1, 0)
}

-- create a cube that spans p0 and p1 inclusive
function csg_lib.create_cube(p0, p1, tag)
   assert(p0 and p1)
   local min, max = Point3(p0), Point3(p1)
   tag = tag or 0

   for _, d in ipairs({ 'x', 'y', 'z'}) do
      if min[d] > max[d] then
         min[d], max[d] = max[d], min[d]
      end
   end

   return Cube3(min, max + Point3.one, tag)
end

local DIMENSIONS = { 'x', 'y', 'z'}

function csg_lib.get_face(cube, normal)
   local dim = nil

   for _, d in ipairs(DIMENSIONS) do
      if normal[d] ~= 0 then
         dim = d
         break
      end
   end
   assert(dim)

   local face = Cube3(cube)

   if normal[dim] > 0 then
      face.min[dim] = face.max[dim] - 1
   else
      face.max[dim] = face.min[dim] + 1
   end

   return face
end

function csg_lib.each_corner_block_in_cube(cube, cb)
   -- block is a corner block when it is part of three or more faces
   csg_lib.each_block_in_cube_with_faces(cube, 3, cb)
end

function csg_lib.each_edge_block_in_cube(cube, cb)
   -- block is an edge block when it is part of two or more faces
   csg_lib.each_block_in_cube_with_faces(cube, 2, cb)
end

function csg_lib.each_face_block_in_cube(cube, cb)
   -- block is a face block when it is part of one or more faces
   csg_lib.each_block_in_cube_with_faces(cube, 1, cb)
end

-- invokes callback when a block in the cube is on min_faces or more
-- min_faces = 3 iterates over all the corner blocks
-- min_faces = 2 iterates over all the edge blocks
-- min_faces = 1 iterates over all the face blocks
function csg_lib.each_block_in_cube_with_faces(cube, min_faces, cb)
    -- subtract one because we want the max terrain block, not the bounding grid line
   local max = cube.max - Point3.one
   local min = cube.min

   -- performance sensitive code, so cache these values to avoid going to C++ every time
   local min_x, min_y, min_z = min.x, min.y, min.z
   local max_x, max_y, max_z = max.x, max.y, max.z

   -- reuse this point to avoid inner loop memory allocation
   local point = Point3()

   local x, y, z, x_face, y_face, z_face, num_faces

   local face_count = function(value, min, max)
      if value == min or value == max then
         return 1
      else
         return 0
      end
   end

   -- can't use for loops because lua doesn't respect changes to the loop counter in the optimization check
   y = min_y
   while y <= max_y do
      y_face = face_count(y, min_y, max_y)
      if y_face + 2 < min_faces then
         -- optimizaton, jump to opposite face if constraint cannot be met
         y = max_y
         y_face = 1
      end

      z = min_z
      while z <= max_z do
         z_face = face_count(z, min_z, max_z)
         if y_face + z_face + 1 < min_faces then
            -- optimizaton, jump to opposite face if constraint cannot be met
            z = max_z
            z_face = 1
         end

         x = min_x
         while x <= max_x do
            x_face = face_count(x, min_x, max_x)
            if x_face + y_face + z_face < min_faces then
               -- optimizaton, jump to opposite face if constraint cannot be met
               x = max_x
               x_face = 1
            end

            num_faces = x_face + y_face + z_face

            if num_faces >= min_faces then
               point:set(x, y, z)
               --log:spam('cube iterator: %s', point)
               cb(point)
            end
            x = x + 1
         end
         z = z + 1
      end
      y = y + 1
   end   
end

function csg_lib.get_aligned_cube(cube, xz_cell_size, y_cell_size)
   local min, max = cube.min, cube.max
   local aligned_min = Point3(
         math.floor(min.x / xz_cell_size) * xz_cell_size,
         math.floor(min.y / y_cell_size)  * y_cell_size,
         math.floor(min.z / xz_cell_size) * xz_cell_size
      )
   local aligned_max = Point3(
         math.ceil(max.x / xz_cell_size) * xz_cell_size,
         math.ceil(max.y / y_cell_size)  * y_cell_size,
         math.ceil(max.z / xz_cell_size) * xz_cell_size
      )
   return Cube3(aligned_min, aligned_max)
end

function csg_lib.create_adjacent_columns(point, y_min, y_max)
   local region = Region3()
   local cube = Cube3(point)
   cube.min.y = y_min
   cube.max.y = y_max

   for _, direction in ipairs(csg_lib.XZ_DIRECTIONS) do
      region:add_unique_cube(cube:translated(direction))
   end

   return region
end

function csg_lib.get_contiguous_regions(parent_region)
   local PointClass, RegionClass
   if radiant.util.is_a(parent_region, Region3) then
      PointClass, RegionClass = Point3, Region3
   else
      PointClass, RegionClass = Point2, Region2
   end

   local sub_regions = {}

   for cube in parent_region:each_cube() do
      local indicies = {}

      for index, sub_region in ipairs(sub_regions) do
         -- create a list of regions adjacent to this cube
         if csg_lib._cube_touches_region(cube, sub_region, PointClass) then
            table.insert(indicies, index)
         end
      end

      local first_index = next(indicies)

      if not first_index then
         -- cube wasn't adjacent to any existing region, so create a new one
         local new_sub_region = RegionClass(cube)
         table.insert(sub_regions, new_sub_region)
      else
         -- get the dominant region that will absorb the other regions
         local dominant_region = sub_regions[first_index]
         dominant_region:add_unique_cube(cube)
         -- remove the dominant region from the list of mergable indicies
         table.remove(indicies, 1)

         -- merge all the subordinate regions into the dominant region
         for _, index in ipairs(indicies) do
            local sub_region = sub_regions[index]
            dominant_region:add_unique_region(sub_region)
         end

         -- remove the subordinate regions that have been merged
         -- iterate from largest index to smallest 
         for i = #indicies, 1, -1 do
            local index = indicies[i]
            table.remove(sub_regions, index)
         end
      end
   end

   return sub_regions
end

function csg_lib._cube_touches_region(cube, region, PointClass)
   assert(PointClass) -- Must be Point2 or Point3

   local x_inflated = cube:inflated(PointClass.unit_x)
   if region:intersects_cube(x_inflated) then
      return true
   end

   if PointClass == Point3 then
      local z_inflated = cube:inflated(PointClass.unit_z)
      if region:intersects_cube(z_inflated) then
         return true
      end
   end

   local y_inflated = cube:inflated(PointClass.unit_y)
   if region:intersects_cube(y_inflated) then
      return true
   end

   return false
end

function csg_lib.get_non_diagonal_xz_inflated_region(region)
   local inflated = region:inflated(Point3.unit_x)
   inflated:add_region(region:inflated(Point3.unit_z))
   return inflated
end

function csg_lib.get_non_diagonal_xyz_inflated_region(region)
   local inflated = region:inflated(Point3.unit_x)
   inflated:add_region(region:inflated(Point3.unit_y))
   inflated:add_region(region:inflated(Point3.unit_z))
   return inflated
end

return csg_lib
