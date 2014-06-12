
local Cube3   = _radiant.csg.Cube3
local Point3  = _radiant.csg.Point3
local Terrain = _radiant.om.Terrain

local WORLD_SIZE = 64
local DEFAULT_FACTION = 'civ'

function create_world(environment) 
   local region3 = _radiant.sim.alloc_region()
   region3:modify(function(r3)
      r3:add_cube(Cube3(Point3(0, -16, 0), Point3(WORLD_SIZE, 0, WORLD_SIZE), Terrain.SOIL_STRATA))
      r3:add_cube(Cube3(Point3(0,   0, 0), Point3(WORLD_SIZE, 1, WORLD_SIZE), Terrain.GRASS))
   end)

   local terrain = radiant._root_entity:add_component('terrain')
   terrain:set_tile_size(WORLD_SIZE)
   terrain:add_tile(Point3(-WORLD_SIZE / 2, 0, -WORLD_SIZE / 2), region3)
end

return create_world