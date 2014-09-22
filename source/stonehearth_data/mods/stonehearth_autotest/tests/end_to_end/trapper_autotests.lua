local Point3 = _radiant.csg.Point3

local trapper_tests = {}

function trapper_tests.basic(autotest)
   local trapping_grounds = autotest.env:create_trapping_grounds(-30, -30, { size = { x = 60, z = 30 }})
   local stockpile = autotest.env:create_stockpile(15, 15, { size = { x = 4, y = 4 }})
   local log = autotest.env:create_entity(7, 7, 'stonehearth:oak_log')
   local trapper = autotest.env:create_person(5, 5, { job = 'trapper' })
   local count = 0

   local component = trapping_grounds:add_component('stonehearth:trapping_grounds')
   component:stop_tasks()
   component.spawn_interval_min = stonehearth.calendar:parse_duration('10m')
   component.spawn_interval_max = stonehearth.calendar:parse_duration('10m')
   component.check_traps_interval = stonehearth.calendar:parse_duration('2h')
   component:start_tasks()

   radiant.events.listen(stockpile, 'stonehearth:item_added', function()
         count = count + 1
         if count == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(100000)
   autotest:fail('trapper test failed to complete on time')
end

return trapper_tests
