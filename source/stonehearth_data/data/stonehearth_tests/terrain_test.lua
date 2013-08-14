
local TerrainTest = class()
local Terrain = _radiant.om.Terrain
local TerrainGenerator = radiant.mods.require('/stonehearth_terrain/terrain_generator.lua')

local CDF_97 = radiant.mods.require('/stonehearth_terrain/wavelet/cdf_97.lua')
local Wavelet = radiant.mods.require('/stonehearth_terrain/wavelet/wavelet.lua')

local RadiantIPoint3 = _radiant.math3d.RadiantIPoint3
local TeraGen = radiant.mods.require('/stonehearth_terrain')


function TerrainTest:__init()
   local tile_size = 256
   --self:_run_unit_tests()
   local terrain_generator = TerrainGenerator(tile_size)
   terrain_generator:generate_tile()
   --terrain_generator:_erosion_test()

   local i
   for i=1, 10, 1 do
      self:place_tree(math.random(1, tile_size), math.random(1, tile_size))
      self:place_citizen(math.random(1, tile_size), math.random(1, tile_size))
   end

   --self:_run_old_sample()
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

return TerrainTest

