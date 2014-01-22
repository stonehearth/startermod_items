local TerrainType = {}

TerrainType.plains    = 'plains'
TerrainType.foothills = 'foothills'
TerrainType.mountains = 'mountains'

function TerrainType.is_valid(value)
   return TerrainType[value] ~= nil
end

return TerrainType
