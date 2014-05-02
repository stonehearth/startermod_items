local Point3 = _radiant.csg.Point3

local sensor_tests = {}

function success_when_matching(autotest, obj, expected)
   local sensor = obj:get_component('sensor_list'):get_sensor('sight')
   local expected_count = #expected
   local trace
   
   -- we cannot call autotest:success() from the trace callback.  it's
   -- impossible to kill the thread from there =(.
   radiant.events.listen_once(obj, 'sensor_test:success', function()
         radiant.log.write('xxxx', 0, 'hi')
         trace:destroy()
         autotest:success()
      end)
      
   local function check_finished()
      local match_count = 0
      local found
      for id, entity in sensor:each_contents() do
         found = false
         for i = 1, expected_count do
            if id == expected[i]:get_id() then
               match_count = match_count + 1
               found = true
               break
            end
         end
         if not found then
            autotest:fail("unexpected entity %s found in sensor", tostring(entity))
         end
      end
      if match_count == expected_count then
         radiant.events.trigger_async(obj, 'sensor_test:success')
      end
   end   
   trace = sensor:trace_contents('autotest')
                 :on_added(check_finished)
                 :on_removed(check_finished)
                 :push_object_state()
   return trace
end


function fail_when_containing(autotest, obj, not_expected)
   local sensor = obj:get_component('sensor_list'):get_sensor('sight')

   -- we cannot call autotest:success() from the trace callback.  it's
   -- impossible to kill the thread from there =(.
   radiant.events.listen_once(obj, 'sensor_test:fail', function(msg)
         autotest:fail(msg)
      end)
      
   local function check_failed(entity_id)
      for _, entity in ipairs(not_expected) do
         if entity:get_id() == entity_id then
            radiant.events.trigger_async(obj, 'sensor_test:fail', string.format('unexpected entity %s in sensor', tostring(entity)))
         end
      end
   end   
   local trace = sensor:trace_contents('autotest')
                       :on_added(check_failed)
                       :push_object_state()
   return trace
end

-- make sure a sensor can detect the ground and itself
function sensor_tests.nop_test(autotest)
   local obj = autotest.env:create_entity(4, 4, 'stonehearth_autotest:entities:5_unit_sensor')

   local trace = success_when_matching(autotest, obj, {
         radiant.get_object(1),
         obj
      })
   
   autotest:sleep(50)
   autotest:fail()
end

-- make sure a sensor doesn't detect things out of range on different tiles
function sensor_tests.out_of_range_test(autotest)
   local obj = autotest.env:create_entity(-4, -4, 'stonehearth_autotest:entities:5_unit_sensor')
   local out_of_range = autotest.env:create_entity(4, 4, 'stonehearth:oak_log')

   local trace = fail_when_containing(autotest, obj, {
         out_of_range
      })
   
   autotest:sleep(50)
   trace:destroy()
   autotest:success()
end

-- make sure a sensor doesn't detect things out of range on the same tile
function sensor_tests.out_of_range_test_same_tile(autotest)
   local obj = autotest.env:create_entity(2, 2, 'stonehearth_autotest:entities:5_unit_sensor')
   local out_of_range = autotest.env:create_entity(9, 9, 'stonehearth:oak_log')

   local trace = fail_when_containing(autotest, obj, {
         out_of_range
      })
   
   autotest:sleep(50)
   trace:destroy()
   autotest:success()
end

function sensor_tests.move_into_range(autotest)
   local obj = autotest.env:create_entity(2, 2, 'stonehearth_autotest:entities:5_unit_sensor')
   local moving_obj = autotest.env:create_entity(9, 9, 'stonehearth:oak_log')

   local trace = fail_when_containing(autotest, obj, {
         moving_obj
      })
   autotest:sleep(50)
   trace:destroy()

   trace = success_when_matching(autotest, obj, {
         radiant.get_object(1),
         obj,
         moving_obj
      })
   radiant.terrain.place_entity(moving_obj, Point3(3, 0, 3))
   autotest:sleep(50)
   autotest:fail()
end

return sensor_tests
