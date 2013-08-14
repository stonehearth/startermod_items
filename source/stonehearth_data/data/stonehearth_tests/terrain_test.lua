
local TerrainTest = class()

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local RadiantIPoint3 = _radiant.math3d.RadiantIPoint3

local TeraGen = radiant.mods.require('/stonehearth_terrain')

local HeightMap = radiant.mods.require('/stonehearth_terrain/height_map.lua')
local TerrainGenerator = radiant.mods.require('/stonehearth_terrain/terrain_generator.lua')
local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')

function TerrainTest:__init()
   local tile_size = 256
   local height_map

   --self:_run_unit_tests()
   local terrain_generator = TerrainGenerator(tile_size)
   height_map = terrain_generator:generate_tile()
   --height_map = terrain_generator:_erosion_test()

   self.foothills_quantization_size = terrain_generator.foothills_quantization_size
   self:_render_height_map_to_terrain(height_map)
   --self:_run_old_sample()

   local i
   for i=1, 10, 1 do
      self:place_tree(math.random(1, tile_size), math.random(1, tile_size))
      self:place_citizen(math.random(1, tile_size), math.random(1, tile_size))
   end
end

function TerrainTest:place_tree(x, z)
   return self:place_item('/stonehearth_trees/entities/oak_tree/medium_oak_tree', x, z)
end

function TerrainTest:place_item(name, x, z)
   local tree = radiant.entities.create_entity(name)
   radiant.terrain.place_entity(tree, RadiantIPoint3(x, 1, z))
   return tree
end

function TerrainTest:place_citizen(x, z, profession)
   local citizen = radiant.mods.load_api('/stonehearth_human_race/').create_entity()
   profession = profession and profession or 'worker'
   local profession = radiant.mods.load_api('/stonehearth_' .. profession .. '_class/').promote(citizen)
   radiant.terrain.place_entity(citizen, RadiantIPoint3(x, 1, z))
   return citizen
end

function TerrainTest:_run_unit_tests()
   CDF_97:_test_perfect_reconstruction()
   Wavelet:_test_perfect_reconstruction()
end

function TerrainTest:_run_old_sample()
   local t = TeraGen()
   t.create()
end

----------

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
   dst:add_cube(Cube3(Point3(rect.min.x, -2,       rect.min.y),
                      Point3(rect.max.x, 0,        rect.max.y),
                Terrain.BEDROCK))

   if height % self.foothills_quantization_size == 0 then
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

