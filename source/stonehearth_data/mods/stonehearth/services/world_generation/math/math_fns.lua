local MathFns = class()

-- rounds towards +infinity even for negative numbers
-- this is the default for most math libraries
function MathFns.round(value)
   return math.floor(value+0.5)
end

function MathFns.quantize(value, step_size)
   local temp = value + step_size*0.5
   local remainder = temp % step_size

	-- if the mod operator is defined properly,
	--   this should never result in a non-integer value
   return temp - remainder
end

-- linear interpolcation / extrapolation
function MathFns.interpolate(x, x1, y1, x2, y2)
   return y1 + (y2-y1) * (x-x1)/(x2-x1)
end

function MathFns.bound(value, min, max)
   if value < min then return min end
   if value > max then return max end
   return value
end

function MathFns.in_bounds(value, min, max)
	if value < min then return false end
	if value > max then return false end
	return true
end

local max_unsigned_int = 4294967295

function MathFns.point_hash(x, y)
   -- from Effective Java (2nd edition)
   local prime = 51
   local value = (prime + MathFns.int_hash(y)) * prime + MathFns.int_hash(x)
   value = value + prime -- don't hash (0,0) to 0
   local hash = value % max_unsigned_int
   return hash
end

function MathFns.int_hash(x)
   local unsigned

   if x >= 0 then unsigned = x
   else           unsigned = max_unsigned_int+1 + x -- recall is negative
   end

   -- simple hash function from Knuth
   local hash = unsigned*2654435761 % max_unsigned_int
   return hash
end

return MathFns
