local TerrainTest = class()

function TerrainTest:__init()
   local wgs = stonehearth.world_generation
   local seed = os.time() % (2^32)
   local blueprint

   wgs:initialize(seed, true)

   blueprint = wgs.blueprint_generator:generate_blueprint(11, 11, seed)
   --blueprint = wgs.blueprint_generator:get_empty_blueprint(2, 2) -- (2,2) is minimum size
   --blueprint = wgs.blueprint_generator:get_static_blueprint()

   wgs:set_blueprint(blueprint)
   wgs:generate_tiles(6, 6, 2)
end

return TerrainTest
