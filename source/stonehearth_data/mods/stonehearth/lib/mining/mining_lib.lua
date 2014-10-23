local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local mining_lib = {}

function mining_lib.get_aligned_cube(cube, xz_align, y_align)
   local min, max = cube.min, cube.max
   local aligned_min = Point3(
         math.floor(min.x / xz_align) * xz_align,
         math.floor(min.y / y_align)  * y_align,
         math.floor(min.z / xz_align) * xz_align
      )
   local aligned_max = Point3(
         math.ceil(max.x / xz_align) * xz_align,
         math.ceil(max.y / y_align)  * y_align,
         math.ceil(max.z / xz_align) * xz_align
      )
   return Cube3(aligned_min, aligned_max)
end

return mining_lib
