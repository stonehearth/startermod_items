local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local mining_lib = {}

function mining_lib.get_aligned_cube(cube, xz_cell_size, y_cell_size)
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

return mining_lib
