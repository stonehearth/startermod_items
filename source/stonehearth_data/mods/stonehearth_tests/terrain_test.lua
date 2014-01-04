local TerrainTest = class()

function TerrainTest:__init()
   local wgs = stonehearth.world_generation
   local seed = os.time() % (2^32)
   local wg = wgs:create_world(true, seed)
end

return TerrainTest
