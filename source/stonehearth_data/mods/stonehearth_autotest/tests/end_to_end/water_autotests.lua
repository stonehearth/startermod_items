local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local water_tests = {}

local function add_cube_to_terrain(x, z, width, length, height, tag)
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y
   local cube = Cube3(
         Point3(x, y, z),
         Point3(x + width, y + height, z + length),
         tag
      )
   radiant.terrain.add_cube(cube)
   return cube
end

local function remove_cube_from_terrain(x, z, width, length, depth)
   local y = radiant.terrain.get_point_on_terrain(Point3(x, 0, z)).y
   local cube = Cube3(
         Point3(x, y - depth, z),
         Point3(x + width, y, z + length)
      )
   radiant.terrain.subtract_cube(cube)
   return cube
end

local function create_lake(x, z, width, length, depth, water_height)
   local cube = remove_cube_from_terrain(x, z, width, length, depth)
   cube.max.y = cube.min.y + math.floor(water_height) + 1
   local water_region = Region3(cube)
   stonehearth.hydrology:create_water_body_with_region(water_region, water_height)
end

local function schedule_command(callback)
   radiant.events.listen_once(radiant, 'radiant:gameloop:end', callback)
end

local function clean_up()
   local water_bodies = stonehearth.hydrology:get_water_bodies()
   for id, entity in pairs(water_bodies) do
      stonehearth.hydrology:destroy_water_body(entity)
   end
end

function water_tests.simple_merge1(autotest)
   create_lake(10, 10, 5, 5, 5, 4.5)
   remove_cube_from_terrain(16, 10, 5, 5, 5)

   schedule_command(function()
         radiant.terrain.subtract_point(Point3(15, 5, 12))
      end)

   radiant.events.listen(stonehearth.hydrology, 'stonehearth:hydrology:water_bodies_merged', function()
         local num_water_bodies = stonehearth.hydrology:num_water_bodies()
         local num_channels = stonehearth.hydrology:num_channels()
         if num_water_bodies == 1 and num_channels == 0 then
            clean_up()
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
end

function water_tests.simple_merge2(autotest)
   create_lake(10, 10, 5, 5, 5, 4.5)
   remove_cube_from_terrain(16, 10, 5, 5, 5)

   schedule_command(function()
         radiant.terrain.subtract_point(Point3(15, 6, 12))
      end)

   radiant.events.listen(stonehearth.hydrology, 'stonehearth:hydrology:water_bodies_merged', function()
         local num_water_bodies = stonehearth.hydrology:num_water_bodies()
         local num_channels = stonehearth.hydrology:num_channels()
         if num_water_bodies == 1 and num_channels == 0 then
            clean_up()
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
end

function water_tests.simple_merge3(autotest)
   create_lake(10, 10, 5, 5, 5, 4.5)
   create_lake(16, 10, 5, 5, 5, 1.5)

   schedule_command(function()
         radiant.terrain.subtract_point(Point3(15, 6, 12))
      end)

   radiant.events.listen(stonehearth.hydrology, 'stonehearth:hydrology:water_bodies_merged', function()
         local num_water_bodies = stonehearth.hydrology:num_water_bodies()
         local num_channels = stonehearth.hydrology:num_channels()
         if num_water_bodies == 1 and num_channels == 0 then
            clean_up()
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
end

return water_tests
