local Point3  = _radiant.csg.Point3
local Cube3   = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local WORLD_SIZE = 64

function create_world(environment)
   radiant.terrain.set_config_file('stonehearth:terrain_block_config')

   local region3 = Region3()
   region3:add_cube(Cube3(Point3(0, -2, 0), Point3(WORLD_SIZE, 0, WORLD_SIZE), block_types.bedrock))
   region3:add_cube(Cube3(Point3(0, 0, 0), Point3(WORLD_SIZE, 9, WORLD_SIZE), block_types.soil_dark))
   region3:add_cube(Cube3(Point3(0, 9, 0), Point3(WORLD_SIZE, 10, WORLD_SIZE), block_types.grass))

   local terrain = radiant._root_entity:add_component('terrain')
   local offset = Point3(-WORLD_SIZE / 2, 0, -WORLD_SIZE / 2)
   terrain:add_tile(region3:translated(offset))
end

return create_world