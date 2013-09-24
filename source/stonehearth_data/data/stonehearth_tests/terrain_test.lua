
local MicroWorld = require 'lib.micro_world'
local TerrainTest = class(MicroWorld)

local HeightMap = radiant.mods.require('stonehearth_terrain.height_map')
local Array2D = radiant.mods.require('stonehearth_terrain.array_2D')
local TerrainType = radiant.mods.require('stonehearth_terrain.terrain_type')
local TerrainGenerator = radiant.mods.require('stonehearth_terrain.terrain_generator')
local Landscaper = radiant.mods.require('stonehearth_terrain.landscaper')
local HeightMapRenderer = radiant.mods.require('stonehearth_terrain.height_map_renderer')
local Timer = radiant.mods.require('stonehearth_debugtools.timer')

function TerrainTest:__init()
   --run_unit_tests()
   --run_timing_tests()

   self[MicroWorld]:__init()

   self._terrain_generator = TerrainGenerator()
   self._landscaper = Landscaper(self._terrain_generator.terrain_info)

   local use_async = true
   --self:tesselator_test()
   --self:tree_test()
   --self:create_world()

   local terrain_thread = function()
      local timer = Timer(Timer.CPU_TIME)
      timer:start()
      self._terrain_generator:set_async(use_async)
      self:create_multi_zone_world()
      self._terrain_generator:set_async(false)
      timer:stop()
      radiant.log.info('World generation time: %.3fs', timer:seconds())
   end

   if use_async then
      radiant.create_background_task('generating terrain', terrain_thread)     
   else
      terrain_thread()
   end
end

function TerrainTest:tesselator_test()
   HeightMapRenderer.tesselator_test()   
end

function TerrainTest:tree_test()
   math.randomseed(4)
   local height_map = HeightMap(256, 256)
   height_map:clear(2)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)
   self._landscaper:place_trees(height_map)
end

function TerrainTest:create_world()
   local height_map

   height_map = self._terrain_generator:generate_zone(TerrainType.Foothills)
   HeightMapRenderer.render_height_map_to_terrain(height_map, self._terrain_generator.terrain_info)

   --self._landscaper:place_trees(height_map)
   --self:_place_people(height_map)
end

function TerrainTest:create_multi_zone_world()
   local zones = self:_create_world_blueprint()
   local num_zones_x = zones.width
   local num_zones_y = zones.height
   local terrain_info = self._terrain_generator.terrain_info
   local zone_size = self._terrain_generator.zone_size
   local timer = Timer(Timer.CPU_TIME)
   local i, j, offset_x, offset_y, zone_map, micro_map, terrain_type

   for j=1, num_zones_y do
      for i=1, num_zones_x do
         terrain_type = zones:get(i, j).terrain_type
         zone_map, micro_map = self._terrain_generator:generate_zone(terrain_type, zones, i, j)
         zones:set(i, j, micro_map)
         -- local zone_map = HeightMap(256, 256)
         -- zone_map:clear(2) -- CHECKCHECK

         offset_x = (i-1)*zone_size
         offset_y = (j-1)*zone_size

         timer:start()
         HeightMapRenderer.render_height_map_to_terrain(zone_map, terrain_info, offset_x, offset_y)
         timer:stop()
         radiant.log.info('HeightMapRenderer time: %.3fs', timer:seconds())

         self:at(1000, function()
            --timer:start()
            self._landscaper:place_trees(zone_map, offset_x, offset_y)
            --self:_place_people(world_map)
            --timer:stop()
            --radiant.log.info('Landscaper time: %.3fs', timer:seconds())
         end)
      end
   end
end

function TerrainTest:_create_world_blueprint()
   local zones = Array2D(3, 3)
   local zone_info
   local i, j

   for j=1, zones.height do
      for i=1, zones.width do
         zone_info = {
            terrain_type = TerrainType.Plains
         }
         zone_info.generated = false
         zones:set(i, j, zone_info)
      end
   end

   -- zones:get(1, 1).terrain_type = TerrainType.Mountains
   -- zones:get(2, 1).terrain_type = TerrainType.Foothills

   -- zones:get(1, 2).terrain_type = TerrainType.Foothills
   -- zones:get(2, 2).terrain_type = TerrainType.Plains

   zones:get(1, 1).terrain_type = TerrainType.Mountains
   zones:get(2, 1).terrain_type = TerrainType.Mountains
   zones:get(3, 1).terrain_type = TerrainType.Foothills

   zones:get(1, 2).terrain_type = TerrainType.Mountains
   zones:get(2, 2).terrain_type = TerrainType.Foothills
   zones:get(3, 2).terrain_type = TerrainType.Plains

   zones:get(1, 3).terrain_type = TerrainType.Foothills
   zones:get(2, 3).terrain_type = TerrainType.Plains
   zones:get(3, 3).terrain_type = TerrainType.Plains

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

