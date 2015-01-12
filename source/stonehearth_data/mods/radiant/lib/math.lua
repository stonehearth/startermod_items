local Point3 = _radiant.csg.Point3
local Quaternion = _radiant.csg.Quaternion

-- naming this 'RadiantMath' to have more explicit differentiation from lua's 'math' table.
-- externally this is exposed as radiant.math
local RadiantMath = {}

RadiantMath.MAX_UINT32 = 2^32-1
RadiantMath.MAX_INT32 = 2^31-1
RadiantMath.MIN_INT32 = -2^31

-- works for negative numbers
-- values ending in .5 round towards +infinity
-- note that floor rounds towards -infinity
function RadiantMath.round(value)
   return math.floor(value+0.5)
end

-- works for negative numbers
-- midpoints round towards +infinity
function RadiantMath.quantize(value, step_size)
   local temp = value + step_size*0.5
   local remainder = temp % step_size
   return temp - remainder
end

-- linear interpolation / extrapolation
function RadiantMath.interpolate(x, x1, y1, x2, y2)
   return y1 + (y2-y1) * (x-x1)/(x2-x1)
end

function RadiantMath.bound(value, min, max)
   if value < min then return min end
   if value > max then return max end
   return value
end

function RadiantMath.in_bounds(value, min, max)
	if value < min then return false end
	if value > max then return false end
	return true
end

function RadiantMath.rotate_about_y_axis(point, degrees)
   local radians = math.rad(degrees)
   local q = Quaternion(Point3.unit_y, radians)
   return q:rotate(point)
end

function RadiantMath.random_xz_unit_vector(rng)
   rng = rng or _radiant.csg.get_default_rng()

   local angle = rng:get_real(0, 360)
   return RadiantMath.rotate_about_y_axis(Point3.unit_x, angle)
end

function RadiantMath.intersect_plane_with_line(line_start, line_dir, plane_point, plane_normal)
   local n_dot_l = line_dir:dot(plane_normal)
   if n_dot_l == 0 then
      return nil
   end

   local t = (line_start - plane_point):dot(plane_normal) / n_dot_l

   return line_start + (line_dir * t)
end

return RadiantMath
