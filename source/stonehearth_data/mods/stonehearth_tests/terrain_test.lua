local TerrainTest = class()

function TerrainTest:__init()
   local wgs = radiant.mods.load('stonehearth').world_generation
   local seed = os.time() % (2^32)
   --local wg = wgs:create_world(true, seed)
   local wg = wgs:create_world(true, 1)
end

return TerrainTest
