local HabitatType = {}

-- initial enumerations
-- these will almost certainly change
HabitatType.occupied  = 'occupied'
HabitatType.plains    = 'plains'
HabitatType.foothills = 'foothills'
HabitatType.mountains = 'mountains'
HabitatType.forest    = 'forest'

function HabitatType.is_valid(value)
   return HabitatType[value] ~= nil
end

return HabitatType
