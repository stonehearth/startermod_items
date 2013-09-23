
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
local Landscaper = radiant.mods.require('/stonehearth_terrain/landscaper.lua')
local HeightMapRenderer = radiant.mods.require('/stonehearth_terrain/height_map_renderer.lua')
local Timer = radiant.mods.require('/stonehearth_debugtools/timer.lua')

function TerrainTest:__init()
   --run_unit_tests()
   --run_timing_tests()

   self[MicroWorld]:__init()

   local timer = Timer(Timer.CPU_TIME)
   timer:start()

   self._terrain_generator = TerrainGenerator()

   --self:tesselator_test()
   --self:tree_test()
   --self:create_world()
   self:create_multi_zone_world()

   timer:stop()
   radiant.log.info('World generation time: %.3fs', timer:seconds())
end

function TerrainTest:tesselator_test()
   HeightMapRenderer.tesselator_test()   
end

function TerrainTest:tree_test()
   math.randomseed(2)
   local height_map = HeightMap(256, 256)
   height_map:clear(8)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)
   Landscaper:place_trees(height_map, self._terrain_generator.terrain_info)
end

function TerrainTest:create_world()
   local height_map

   --height_map = terrain_generator:_erosion_test()
   height_map = self._terrain_generator:generate_zone(TerrainType.Foothills)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)

   Landscaper:place_trees(height_map, self._terrain_generator.terrain_info)
   --self:_place_people(height_map)
end

function TerrainTest:create_multi_zone_world()
   local zones = self:_create_world_blueprint()
   local num_zones_x = zones.width
   local num_zones_y = zones.height
   local zone_size = self._terrain_generator.zone_size
   local world_map = HeightMap(zone_size*num_zones_x, zone_size*num_zones_y)
   local i, j, zone_map, micro_map, terrain_type

   world_map:clear(1)

   for j=1, num_zones_y do
      for i=1, num_zones_x do
         terrain_type = zones:get(i, j).terrain_type
         zone_map, micro_map = self._terrain_generator:generate_zone(terrain_type, zones, i, j)
         zone_map:copy_block(world_map, zone_map,
            (i-1)*zone_size+1, (j-1)*zone_size+1, 1, 1, zone_size, zone_size)
         zones:set(i, j, micro_map)
      end
   end

   local timer = Timer(Timer.CPU_TIME)
   timer:start()
   HeightMapRenderer.render_height_map_to_terrain(world_map, self._terrain_generator.terrain_info)
   timer:stop()
   radiant.log.info('HeightMapRenderer time: %.3fs', timer:seconds())

   timer:start()
   Landscaper:place_trees(world_map, self._terrain_generator.terrain_info)
   --self:_place_people(world_map)
   timer:stop()
   radiant.log.info('Landscaper time: %.3fs', timer:seconds())
end

function TerrainTest:_create_world_blueprint()
   local zones = Array2D(2, 2)
   local zone_info
   local i, j

   for j=1, zones.height do
      for i=1, zones.width do
         zone_info = {}
         zone_info.generated = false
         zones:set(i, j, zone_info)
      end
   end

   zones:get(1, 1).terrain_type = TerrainType.Mountains
   zones:get(2, 1).terrain_type = TerrainType.Foothills

   zones:get(1, 2).terrain_type = TerrainType.Foothills
   zones:get(2, 2).terrain_type = TerrainType.Plains

   -- zones:get(1, 1).terrain_type = TerrainType.Mountains
   -- zones:get(2, 1).terrain_type = TerrainType.Mountains
   -- zones:get(3, 1).terrain_type = TerrainType.Foothills

   -- zones:get(1, 2).terrain_type = TerrainType.Mountains
   -- zones:get(2, 2).terrain_type = TerrainType.Foothills
   -- zones:get(3, 2).terrain_type = TerrainType.Plains

   -- zones:get(1, 3).terrain_type = TerrainType.Foothills
   -- zones:get(2, 3).terrain_type = TerrainType.Plains
   -- zones:get(3, 3).terrain_type = TerrainType.Plains

   return zones
end

function TerrainTest:_place_people(height_map)
   local margin = 4
   local width = height_map.width
   local height = height_map.height
   local num_people = width*height / 16
   local i
   num_people = 10
   for i=1, num_people do
      self:place_citizen(math.random(1+margin, width-margin),
                         math.random(1+margin, height-margin))
   end
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

