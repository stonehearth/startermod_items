local TerrainTest = class()

function TerrainTest:__init()
   local wgs = radiant.mods.load('stonehearth').world_generation
   local seed = os.time() % (2^32)
   wgs:initialize(seed, true)
   --wgs:initialize(1, true)
   wgs:create_blueprint(5, 5)
   wgs:generate_all_tiles()
end

return TerrainTest
