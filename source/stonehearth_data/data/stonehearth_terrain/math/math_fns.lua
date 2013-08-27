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

return MathFns
