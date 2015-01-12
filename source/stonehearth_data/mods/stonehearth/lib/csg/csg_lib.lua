local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local csg_lib = {}

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
   local x, y, z, x_face, y_face, z_face, num_faces
   -- reuse this point to avoid inner loop memory allocation
   local point = Point3()

   local face_count = function(value, min, max)
      if value == min or value == max then
         return 1
      else
         return 0
      end
   end

   -- can't use for loops because lua doesn't respect changes to the loop counter in the optimization check
   y = min.y
   while y <= max.y do
      y_face = face_count(y, min.y, max.y)
      if y_face + 2 < min_faces then
         -- optimizaton, jump to opposite face if constraint cannot be met
         y = max.y
         y_face = 1
      end

      z = min.z
      while z <= max.z do
         z_face = face_count(z, min.z, max.z)
         if y_face + z_face + 1 < min_faces then
            -- optimizaton, jump to opposite face if constraint cannot be met
            z = max.z
            z_face = 1
         end

         x = min.x
         while x <= max.x do
            x_face = face_count(x, min.x, max.x)
            if x_face + y_face + z_face < min_faces then
               -- optimizaton, jump to opposite face if constraint cannot be met
               x = max.x
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

   local add_xz_column = function(region, x, z, y_min, y_max)
      region:add_cube(Cube3(
            Point3(x,   y_min, z),
            Point3(x+1, y_max, z+1)
         ))
   end

   add_xz_column(region, point.x-1, point.z,   y_min, y_max)
   add_xz_column(region, point.x+1, point.z,   y_min, y_max)
   add_xz_column(region, point.x,   point.z-1, y_min, y_max)
   add_xz_column(region, point.x,   point.z+1, y_min, y_max)

   return region
end

return csg_lib
