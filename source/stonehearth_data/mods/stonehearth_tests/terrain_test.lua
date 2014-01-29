local TerrainTest = class()

function TerrainTest:__init()
   local wgs = stonehearth.world_generation
   local seed = os.time() % (2^32)
   --wgs:initialize(true, seed)
   wgs:initialize(true, 1)
   wgs:create_world()
end

return TerrainTest
