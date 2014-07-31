local Point3f = _radiant.csg.Point3f

local sleep_tests = {}

function sleep_tests.sleep_on_ground(autotest)
   local worker = autotest.env:create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = stonehearth.constants.sleep.EXHAUSTION },
   })

   autotest.util:succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:fail_if_expired(60 * 1000)
end

function sleep_tests.sleep_in_new_bed(autotest)
   autotest.env:create_entity(10, 10, 'stonehearth:comfy_bed', { force_iconic = false })
   local worker = autotest.env:create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 5 },
   })

   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   autotest.util:succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:fail_if_attribute_above(worker, 'sleepiness', 20)
   autotest.util:fail_if_expired(60 * 1000)
end

function sleep_tests.sleep_in_own_bed(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   local worker = autotest.env:create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 5 },
   })
   -- lease it...
   local bed = autotest.env:create_entity(10, 10, 'stonehearth:comfy_bed', { force_iconic = false })
   bed:add_component('stonehearth:lease'):acquire('stonehearth:bed', worker)

   autotest.util:succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:fail_if_attribute_above(worker, 'sleepiness', 25)
   autotest.util:fail_if_expired(60 * 1000)
end

function sleep_tests.not_sleep_in_others_beds(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   local worker = autotest.env:create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 5 },
   })
   local trapper = autotest.env:create_person(4, 4, { profession = 'trapper'})

   -- lease it to someone else who's not sleepy!
   local bed = autotest.env:create_entity(10, 10, 'stonehearth:comfy_bed', { force_iconic = false })
   bed:add_component('stonehearth:lease'):acquire('stonehearth:bed', trapper)

   autotest.util:fail_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util:succeed_if_attribute_above(worker, 'sleepiness', 6)
   autotest.util:fail_if_expired(60 * 1000)
end

return sleep_tests
