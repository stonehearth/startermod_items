
local MicroWorld = radiant.mods.require('/stonehearth_tests/lib/micro_world.lua')
local TerrainTest = class(MicroWorld)

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local TeraGen = radiant.mods.require('/stonehearth_terrain')

local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local Array2D = radiant.mods.require('/stonehearth_terrain/array_2D.lua')
local TerrainGenerator = radiant.mods.require('/stonehearth_terrain/terrain_generator.lua')
local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')
local BoundaryNormalizingFilter = radiant.mods.require('/stonehearth_terrain/filter/boundary_normalizing_filter.lua')
local FilterFns = radiant.mods.require('/stonehearth_terrain/filter/filter_fns.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
local InverseGaussianRandom = radiant.mods.require('/stonehearth_terrain/math/inverse_gaussian_random.lua')
local Timer = radiant.mods.require('/stonehearth_terrain/timer.lua')

function TerrainTest:__init()
   --self:_run_timing_tests()
   --self:_run_unit_tests()
   
   self[MicroWorld]:__init()

   self._terrain_generator = TerrainGenerator()

   self:create_world()
   --self:create_multi_zone_world()
end

function TerrainTest:create_world()
   local height_map

   --height_map = terrain_generator:_erosion_test()
   height_map = self._terrain_generator:generate_zone()
   self:_render_height_map_to_terrain(height_map)
   self:decorate_landscape()
end

function TerrainTest:create_multi_zone_world()
   local zone_size = self._terrain_generator.zone_size
   local world_map = HeightMap(zone_size*2, zone_size*2)
   local zones = Array2D(3, 3)
   local zone_map
   local micro_map

   world_map:process_map(function () return 1 end)

   zone_map = self._terrain_generator:generate_zone(zones, 2, 2)
   zone_map:copy_block(world_map, zone_map, 1, 1, 1, 1, zone_size, zone_size)
   micro_map = self._terrain_generator:_create_micro_map(zone_map)
   zones:set(2, 2, micro_map)

   zone_map = self._terrain_generator:generate_zone(zones, 2, 3)
   zone_map:copy_block(world_map, zone_map, 1, zone_size, 1, 1, zone_size, zone_size)
   micro_map = self._terrain_generator:_create_micro_map(zone_map)
   zones:set(2, 3, micro_map)
   self:_render_height_map_to_terrain(world_map)
end

function TerrainTest:create_old_world()
   local t = TeraGen()
   t.create()
end

function TerrainTest:decorate_landscape()
   local zone_size = self._terrain_generator.zone_size
   local i
   for i=1, 20, 1 do
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
   local timer = Timer()
   local iterations = 20000000
   local i, value

   timer.start()

   for i=1, iterations, 1 do
      --value = InverseGaussianRandom.generate(1, 8, (8-1)/4)
      value = GaussianRandom.generate(50, 10)
      --radiant.log.info("%f", value)
   end

   timer.stop()
   radiant.log.info("Duration: %f", timer.duration())
   assert(false)
end

----------

-- delegate to C++ to "tesselate" heightmap into rectangles
function TerrainTest:_render_height_map_to_terrain(height_map)
   local r2 = Region2()
   local r3 = Region3()
   local heightMapCPP = HeightMapCPP(height_map.width, 1) -- Assumes square map!
   local terrain = radiant._root_entity:add_component('terrain')

   self:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   _radiant.csg.convert_heightmap_to_region2(heightMapCPP, r2)

   for rect in r2:contents() do
      if rect.tag > 0 then
         self:_add_land_to_region(r3, rect, rect.tag);         
      end
   end

   terrain:add_region(r3)
end

function TerrainTest:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   local row_offset = 0

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         heightMapCPP:set(i-1, j-1, math.abs(height_map[row_offset+i]))
      end
      row_offset = row_offset + height_map.width
   end
end

function TerrainTest:_add_land_to_region(dst, rect, height)
   local foothills_quantization_size = self._terrain_generator.foothills_quantization_size

   dst:add_cube(Cube3(Point3(rect.min.x, -2, rect.min.y),
                      Point3(rect.max.x,  0, rect.max.y),
                Terrain.BEDROCK))

   if height % foothills_quantization_size == 0 then
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.GRASS))
   else
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.TOPSOIL))
   end
end

return TerrainTest

