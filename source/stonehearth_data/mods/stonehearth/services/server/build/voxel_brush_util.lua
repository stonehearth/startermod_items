local voxel_brush_util = {}
local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3

local MODEL_OFFSET = Point3(0, 0, 0)

function voxel_brush_util.create_brush(brush, origin, normal)
   checks('string', '?Point3', '?Point3')

   if Color3.is_color(brush) then
      return _radiant.voxel.create_color_brush(brush)
   end

   local brush = _radiant.voxel.create_qubicle_brush(brush)
   if origin then
      brush:set_origin(origin)
   end
   if normal then
      brush:set_normal(normal)
   end
   brush:set_clip_whitespace(true)                                   

   return brush
end

return voxel_brush_util
