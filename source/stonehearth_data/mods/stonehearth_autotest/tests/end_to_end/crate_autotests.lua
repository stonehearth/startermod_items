local Point3 = _radiant.csg.Point3

local crate_tests = {}

-- make sure we can simply restock the crate
function crate_tests.restock(autotest)
   autotest.env:create_person(2, 2, { job = 'worker' })
   autotest.env:create_entity(4, 4, 'stonehearth:resources:wood:oak_log')
   autotest.env:create_entity(4, 5, 'stonehearth:resources:wood:oak_log')

   local crate = autotest.env:create_entity(0, 0, 'stonehearth:containers:small_crate', { force_iconic = false })
   local counter = 0
   radiant.events.listen(crate, 'stonehearth:backpack:item_added', function()
         counter = counter + 1
         if counter == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
   autotest:fail('worker did not put all the logs in the crate')
end


return crate_tests
