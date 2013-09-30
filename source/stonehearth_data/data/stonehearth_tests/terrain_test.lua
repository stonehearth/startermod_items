local HeightMap = radiant.mods.require('stonehearth.services.world_generation.height_map')
local TerrainType = radiant.mods.require('stonehearth.services.world_generation.terrain_type')
local TerrainGenerator = radiant.mods.require('stonehearth.services.world_generation.terrain_generator')
local Landscaper = radiant.mods.require('stonehearth.services.world_generation.landscaper')
local HeightMapRenderer = radiant.mods.require('stonehearth.services.world_generation.height_map_renderer')
local Timer = radiant.mods.require('stonehearth_debugtools.timer')

local TerrainTest = class()

function TerrainTest:__init()
   --run_unit_tests()
   --run_timing_tests()
   --self:tesselator_test()
   local wgs = radiant.mods.load('stonehearth').world_generation
   local wg = wgs:create_world(true)
end

function TerrainTest:tesselator_test()
   HeightMapRenderer.tesselator_test()   
end

function TerrainTest:tree_test()
   math.randomseed(1)
   local height_map = HeightMap(256, 256)
   height_map:clear(2)
   self._height_map_renderer:render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)
   self._landscaper:place_trees(height_map)
end

function TerrainTest:create_world()
   local height_map

   height_map = self._terrain_generator:generate_zone(TerrainType.Foothills)
   self._height_map_renderer:render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)

   self._landscaper:place_trees(height_map)
end

function run_unit_tests()
   BoundaryNormalizingFilter._test()
   FilterFns._test()
   CDF_97._test()
   Wavelet._test()
end

function run_timing_tests()
   local abs = math.abs
   local timer = Timer(Timer.CPU_TIME)
   local iterations = 50000000

   timer:start()

   for i = 1, iterations do
      -- do something
   end

   timer:stop()
   radiant.log.info("Duration: %.3fs", timer:seconds())
   radiant.log.info("Iterations/s: %d", iterations/timer:seconds())
   assert(false)
end

function check_wavelet_impulse_function()
   local height_map = HeightMap(8, 8)
   height_map:clear(0)
   height_map:set(4, 4, 100)
   Wavelet.DWT_2D(height_map, 8, 8, 1, 1)
   local dummy = 1
end

return TerrainTest

