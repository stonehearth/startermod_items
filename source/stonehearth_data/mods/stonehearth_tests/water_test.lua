local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterTest = class(MicroWorld)

function WaterTest:__init()
   self[MicroWorld]:__init(256)
   self:create_world()

   local session = self:get_session()

   self:_create_lake(-40, -40, 40, 40, 5, 3.5)
   --self:_create_walled_lake()
   --self:_create_enclosed_wall()

   self:place_citizen(0, 20)
   -- self:place_citizen(2, 20)
   -- self:place_citizen(4, 20)
   -- self:place_citizen(6, 20)

   -- wait for watertight region to update before starting
   stonehearth.calendar:set_timer(10, function()
         --self:_water_test()
      end)
end

function WaterTest:_create_lake(x, z, width, length, depth, water_height)
   local cube = self:_remove_cube_from_terrain(x, z, width, length, depth)
   cube.max.y = cube.min.y + math.floor(water_height) + 1
   local water_region = Region3(cube)
   stonehearth.hydrology:create_water_body_with_region(water_region, water_height)
end

function WaterTest:_create_walled_lake()
   self:_add_cube_to_terrain(-10, -10, 10, 10, 5, 200)
   self:_add_cube_to_terrain(-10, -10, 10, 10, 1, 300)
   self:_create_lake(-9, -9, 8, 8, 5, 3.5)

   --radiant.terrain.subtract_point(Point3(-5, 14, -1))
end

function WaterTest:_create_enclosed_wall()
   local cube = Cube3(
         Point3(-12, 10, -12),
         Point3(3, 16, 3),
         300
      )

   local cube2 = Cube3(
         Point3(-11, 10, -11),
         Point3(2, 16, 2)
      )

   local region = Region3()
   region:add_cube(cube)
   region:subtract_cube(cube2)
   radiant.terrain.add_region(region)
end

function WaterTest:_water_test()
   local location = radiant.terrain.get_point_on_terrain(Point3(-5, 0, -5))

   stonehearth.calendar:set_interval(10, function()
         stonehearth.hydrology:add_water(2, location)
      end)
end

function WaterTest:_add_cube_to_terrain(x, z, width, length, height, tag)
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y
   local cube = Cube3(
         Point3(x, y, z),
         Point3(x + width, y + height, z + length),
         tag
      )
   radiant.terrain.add_cube(cube)
   return cube
end

function WaterTest:_remove_cube_from_terrain(x, z, width, length, depth)
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y
   local cube = Cube3(
         Point3(x, y - depth, z),
         Point3(x + width, y, z + length)
      )
   radiant.terrain.subtract_cube(cube)
   return cube
end

return WaterTest
