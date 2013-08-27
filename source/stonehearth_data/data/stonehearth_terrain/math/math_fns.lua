local MathFns = class()

-- rounds towards +infinity even for negative numbers
-- this is the default for most math libraries
function MathFns.round(value)
   return math.floor(value+0.5)
end

return MathFns
