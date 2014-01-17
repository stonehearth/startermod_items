local TerrainType = {}

-- change these integers to strings when terrain_generator:_calc_std_dev is removed (or fixed)
TerrainType.ocean     = 0
TerrainType.grassland = 1
TerrainType.foothills = 2
TerrainType.mountains = 3

function TerrainType.is_valid(value)
   return TerrainType[value] ~= nil
end

return TerrainType
