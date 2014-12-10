local build_util = require 'stonehearth.lib.build_util'

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local roads_autotests = {}

function _check_sensor_for_entity(sensor_entity, entity)
   local sensor = sensor_entity:get_component('sensor_list'):get_sensor('sight')
   for id, ent in sensor:each_contents() do
      if entity == ent then
         return true
      end
   end
   return false
end

function roads_autotests.follow_road(autotest)
   local person = autotest.env:create_person(-22, -22, { job = 'worker' })   
   autotest.env:create_entity(-24, -24, 'stonehearth:resources:wood:oak_log')
   local stockpile = autotest.env:create_stockpile(24, 24)
   local session = autotest.env:get_player_session()

   local sensor1 = autotest.env:create_entity(4, -24, 'stonehearth_autotest:entities:5_unit_sensor')
   local sensor2 = autotest.env:create_entity(-4, 24, 'stonehearth_autotest:entities:5_unit_sensor')
   local sensor3 = autotest.env:create_entity(25, 25, 'stonehearth_autotest:entities:5_unit_sensor')

   -- create a zig-zag road.
   local road

   stonehearth.build:do_command('add_road', nil, function()
         road = stonehearth.build:add_road(session, 'stonehearth:brick_paved_road', nil, Cube3(Point3(-20, 9, -20), Point3(0, 10, -18)))
      end)
   stonehearth.build:do_command('add_road', nil, function()
         stonehearth.build:add_road(session, 'stonehearth:brick_paved_road', nil, Cube3(Point3(-2, 9, -20), Point3(0, 10, 20)))
      end)
   stonehearth.build:do_command('add_road', nil, function()
         stonehearth.build:add_road(session, 'stonehearth:brick_paved_road', nil, Cube3(Point3(-2, 9, 18), Point3(20, 10, 20)))
      end)  


   local building = build_util.get_building_for(road)
   stonehearth.build:instabuild(building)

   local tripped_1 = false
   local tripped_2 = false
   local tripped_3 = false
   local elapsed_time = 0
   local sleep_time = 100
   while (not tripped_1 or not tripped_2 or not tripped_3) and elapsed_time < 20000 do
      autotest:sleep(sleep_time)
      elapsed_time = elapsed_time + sleep_time

      tripped_1 = tripped_1 or _check_sensor_for_entity(sensor1, person)
      tripped_2 = tripped_2 or _check_sensor_for_entity(sensor2, person)
      tripped_3 = tripped_3 or _check_sensor_for_entity(sensor3, person)
   end

   if not tripped_1 or not tripped_2 or not tripped_3 then
      autotest:fail('did not trip the two sensors')
   end
   autotest:success()
end

return roads_autotests
