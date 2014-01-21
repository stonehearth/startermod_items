local TerrainType = {}

TerrainType.grassland = 'grassland'
TerrainType.foothills = 'foothills'
TerrainType.mountains = 'mountains'

function TerrainType.is_valid(value)
   return TerrainType[value] ~= nil
end

return TerrainType
