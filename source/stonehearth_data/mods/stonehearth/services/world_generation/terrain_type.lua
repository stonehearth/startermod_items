local TerrainType = {}

TerrainType.plains    = 'plains'
TerrainType.foothills = 'foothills'
TerrainType.mountains = 'mountains'

local terrain_order = {
   TerrainType.plains,
   TerrainType.foothills,
   TerrainType.mountains
}

function TerrainType.is_valid(value)
   return TerrainType[value] ~= nil
end

function TerrainType.get_terrain_order()
   return terrain_order
end

return TerrainType
