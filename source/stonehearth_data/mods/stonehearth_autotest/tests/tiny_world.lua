local Point3  = _radiant.csg.Point3
local Cube3   = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Terrain = _radiant.om.Terrain

local WORLD_SIZE = 64
local DEFAULT_FACTION = 'civ'

function create_world(environment) 
   local region3 = Region3()
   region3:add_cube(Cube3(Point3(0, -16, 0), Point3(WORLD_SIZE, 0, WORLD_SIZE), Terrain.SOIL_LIGHT))
   region3:add_cube(Cube3(Point3(0,   0, 0), Point3(WORLD_SIZE, 1, WORLD_SIZE), Terrain.GRASS))

   local terrain = radiant._root_entity:add_component('terrain')
   local offset = Point3(-WORLD_SIZE / 2, 0, -WORLD_SIZE / 2)
   terrain:add_tile(region3:translated(offset))
end

return create_world