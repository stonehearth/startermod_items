local Point3f = _radiant.csg.Point3f

local sleep_tests = {}

function sleep_tests.sleep_on_ground()
   local worker = autotest.env.create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 1000 },
   })

   autotest.util.succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util.fail_if_expired(60 * 1000)
end

function sleep_tests.sleep_in_new_bed()
   autotest.env.create_entity(10, 10, 'stonehearth:comfy_bed')
   local worker = autotest.env.create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 81 },
   })

   autotest.util.succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util.fail_if_attribute_above(worker, 'sleepiness', 119)
   autotest.util.fail_if_expired(60 * 1000)
end

function sleep_tests.sleep_in_own_bed()
   local worker = autotest.env.create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 81 },
   })
   -- lease it...
   local bed = autotest.env.create_entity(10, 10, 'stonehearth:comfy_bed')
   bed:add_component('stonehearth:lease'):acquire('stonehearth:bed', worker)

   autotest.util.succeed_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util.fail_if_attribute_above(worker, 'sleepiness', 119)
   autotest.util.fail_if_expired(60 * 1000)
end

function sleep_tests.not_sleep_in_others_beds()
   local worker = autotest.env.create_person(2, 2, {
      profession = 'worker',
      attributes = { sleepiness = 81 },
   })
   local not_sleepy_trapper = autotest.env.create_person(4, 4, { profession = 'trapper'})

   -- lease it to someone else who's not sleepy!
   local bed = autotest.env.create_entity(10, 10, 'stonehearth:comfy_bed')
   bed:add_component('stonehearth:lease'):acquire('stonehearth:bed', not_sleepy_trapper)

   autotest.util.fail_if_buff_added(worker, 'stonehearth:buffs:sleeping')
   autotest.util.succeed_if_attribute_above(worker, 'sleepiness', 83)
   autotest.util.fail_if_expired(60 * 1000)
end

return sleep_tests
