
local MicroWorld = require 'lib/micro_world'
local TerrainTest = class(MicroWorld)

local TerrainType = radiant.mods.require('/stonehearth_terrain/terrain_type.lua')
local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local Array2D = radiant.mods.require('/stonehearth_terrain/array_2D.lua')
local TerrainGenerator = radiant.mods.require('/stonehearth_terrain/terrain_generator.lua')
local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local BoundaryNormalizingFilter = radiant.mods.require('/stonehearth_terrain/filter/boundary_normalizing_filter.lua')
local FilterFns = radiant.mods.require('/stonehearth_terrain/filter/filter_fns.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
local InverseGaussianRandom = radiant.mods.require('/stonehearth_terrain/math/inverse_gaussian_random.lua')
local Landscaper = radiant.mods.require('/stonehearth_terrain/landscaper.lua')
local HeightMapRenderer = radiant.mods.require('/stonehearth_terrain/height_map_renderer.lua')
local Timer = radiant.mods.require('/stonehearth_debugtools/timer.lua')

function TerrainTest:__init()
   --self:_run_timing_tests()
   --self:_run_unit_tests()

   self[MicroWorld]:__init()

   local timer = Timer(Timer.CPU_TIME)
   timer:start()

   self._terrain_generator = TerrainGenerator()

   --self:create_world()
   self:create_multi_zone_world()

   timer:stop()
   radiant.log.info('Terrain generation time: %.3fs', timer:seconds())
end

function TerrainTest:create_world()
   local height_map

   --height_map = terrain_generator:_erosion_test()
   height_map = self._terrain_generator:generate_zone(TerrainType.Foothills)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)

   Landscaper:place_trees(height_map)
end

function TerrainTest:create_multi_zone_world()
   local zone_size = self._terrain_generator.zone_size
   local world_map = HeightMap(zone_size*2, zone_size*2)
   local zones = Array2D(3, 3)
   local zone_map
   local micro_map

   world_map:clear(1)

   zone_map, micro_map = self._terrain_generator:generate_zone(TerrainType.Mountains, zones, 2, 2)
   zone_map:copy_block(world_map, zone_map, 1, 1, 1, 1, zone_size, zone_size)
   zones:set(2, 2, micro_map)

   zone_map, micro_map = self._terrain_generator:generate_zone(TerrainType.Foothills, zones, 2, 3)
   zone_map:copy_block(world_map, zone_map, 1, zone_size+1, 1, 1, zone_size, zone_size)
   zones:set(2, 3, micro_map)

   zone_map, micro_map = self._terrain_generator:generate_zone(TerrainType.Foothills, zones, 3, 2)
   zone_map:copy_block(world_map, zone_map, zone_size+1, 1, 1, 1, zone_size, zone_size)
   zones:set(3, 2, micro_map)

   zone_map, micro_map = self._terrain_generator:generate_zone(TerrainType.Plains, zones, 3, 3)
   zone_map:copy_block(world_map, zone_map, zone_size+1, zone_size+1, 1, 1, zone_size, zone_size)
   zones:set(3, 3, micro_map)

   HeightMapRenderer.render_height_map_to_terrain(world_map, self._terrain_generator.terrain_info)

   Landscaper:place_trees(world_map)
end

function TerrainTest:create_old_world()
   local t = TeraGen()
   t.create()
end

function TerrainTest:decorate_landscape()
   local zone_size = self._terrain_generator.zone_size
   local i
   for i=1, 20 do
      self:place_tree(math.random(1, zone_size), math.random(1, zone_size))
      self:place_citizen(math.random(1, zone_size), math.random(1, zone_size))
   end
end

function TerrainTest:_run_unit_tests()
   BoundaryNormalizingFilter._test()
   FilterFns._test()
   CDF_97._test()
   Wavelet._test()
end

function TerrainTest:_run_timing_tests()
   local timer = Timer(Timer.CPU_TIME)
   local iterations = 20000000
   local i, value

   timer.start()

   for i=1, iterations do
      --value = InverseGaussianRandom.generate(1, 8, (8-1)/4)
      value = GaussianRandom.generate(50, 10)
      --radiant.log.info("%f", value)
   end

   timer.stop()
   radiant.log.info("Duration: %f", timer.duration())
   assert(false)
end

function TerrainTest._check_wavelet_impulse_function()
   local height_map = HeightMap(8, 8)
   height_map:clear(0)
   height_map:set(4, 4, 100)
   Wavelet.DWT_2D(height_map, 8, 8, 1, 1)
   local dummy = 1
end


return TerrainTest

