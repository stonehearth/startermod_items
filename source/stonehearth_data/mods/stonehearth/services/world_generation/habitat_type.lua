local StringFns = require 'services.world_generation.string_fns'

local HabitatType = {}

-- initial enumerations
-- these will almost certainly change
HabitatType.occupied  = 'occupied'
HabitatType.grassland = 'grassland'
HabitatType.foothills = 'foothills'
HabitatType.mountains = 'mountains'
HabitatType.forest    = 'forest'

function HabitatType.is_valid(value)
   return HabitatType[value] ~= nil
end

return HabitatType
