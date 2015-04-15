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

   --self:_create_lake()
   self:_create_walled_lake()
   self:_create_enclosed_wall()
   --self:_create_depression()

   --self:place_citizen(0, 20)

   -- wait for watertight region to update before starting
   stonehearth.calendar:set_timer(10, function()
         self:_water_test()
      end)
end

function WaterTest:_create_lake()
   local cube = self:_remove_cube_from_terrain(0, 0, 16, 16, 5)
   self:_fill_cube(cube, 4.5)
end

function WaterTest:_create_walled_lake()
   local height = 15
   local cube = Cube3(
         Point3(-10, 10, -10),
         Point3(0, 10, 0)
      )

   cube.max.y = height-1
   cube.tag = 200

   radiant.terrain.add_cube(cube)

   cube.min.y = cube.max.y
   cube.max.y = height
   cube.tag = 300

   radiant.terrain.add_cube(cube)

   cube.min.y = 10
   cube.max.y = height
   local inset = cube:inflated(Point3(-1, 0, -1))

   radiant.terrain.subtract_cube(inset)
   radiant.terrain.subtract_point(Point3(-5, 14, -1))

   self:_fill_cube(inset, 4.5)
end

function WaterTest:_create_depression()
   local cube = Cube3(
         Point3(-5, 9, 5),
         Point3(0, 10, 10)
      )

   radiant.terrain.subtract_cube(cube)
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

function WaterTest:_add_cube_to_terrain(x, z, width, length, height)
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y
   local cube = Cube3(
         Point3(x, y, z),
         Point3(x + width, y + height, z + length)
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

function WaterTest:_fill_cube(cube, height)
   local wetting_constant = 0.25
   local location = _radiant.csg.get_cube_centroid(cube):to_closest_int()
   local layer_volume = cube:get_area() / cube:get_size().y
   local water_volume = layer_volume * (height + wetting_constant)
   self:_add_water(water_volume, location)
end

function WaterTest:_add_water(volume, location)
   stonehearth.calendar:set_timer(10, function()
         local resolved_location = radiant.terrain.get_point_on_terrain(location)
         stonehearth.hydrology:add_water(volume, resolved_location)
      end)
end

return WaterTest
