local TerrainTest = class()

function TerrainTest:__init()
   local wgs = radiant.mods.load('stonehearth').world_generation
   local wg = wgs:create_world(true)
end

return TerrainTest
