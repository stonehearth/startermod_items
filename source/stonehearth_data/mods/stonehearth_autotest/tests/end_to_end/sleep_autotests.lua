local Point3 = _radiant.csg.Point3

local sleep_tests = {}

function sleep_tests.sleep_on_ground(autotest)
   local worker = autotest.env:create_person(2, 2, {
      job = 'worker',
      attributes = { sleepiness = stonehearth.constants.sleep.EXHAUSTION },
   })

   autotest.util:succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:fail_if_expired(60 * 1000)
end

function sleep_tests.sleep_in_new_bed(autotest)
   autotest.env:create_entity(10, 10, 'stonehearth:furniture:comfy_bed', { force_iconic = false })
   local worker = autotest.env:create_person(2, 2, {
      job = 'worker',
      attributes = { sleepiness = 5 },
   })

   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   autotest.util:succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:fail_if_attribute_above(worker, 'sleepiness', 20)
   autotest.util:fail_if_expired(60 * 1000)
end

return sleep_tests
