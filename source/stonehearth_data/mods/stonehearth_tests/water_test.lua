local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('water')

local WaterTest = class(MicroWorld)

function WaterTest:__init()
   self[MicroWorld]:__init(128)
   self:create_world()

   self:_create_lake()
   --self:_create_enclosed_wall()
   --self:_create_depression()
   self:_water_test()

   local session = self:get_session()
end

function WaterTest:_create_lake()
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

   local location = radiant.terrain.get_point_on_terrain(Point3(-5, 0, -5))
   self._water_body = stonehearth.hydrology:create_water_body(location)

   stonehearth.hydrology:add_water(64/4 + 64*4, location, self._water_body)
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
    stonehearth.calendar:set_interval(10, function()
         local location = radiant.terrain.get_point_on_terrain(Point3(-5, 0, -5))
         stonehearth.hydrology:add_water(1, location)
      end)
end

return WaterTest
