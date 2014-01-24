local TerrainTest = class()

function TerrainTest:__init()
   local wgs = radiant.mods.load('stonehearth').world_generation
   local seed = os.time() % (2^32)
   --wgs:initialize(true, seed)
   wgs:initialize(1, true)
   wgs:create_world()
end

return TerrainTest
