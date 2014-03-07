local Point3f = _radiant.csg.Point3f

local food_tests = {}

local function make_hungry_worker(hunger)
   hunger = hunger or 1000  -- quite hungry, indeed!
   local worker = autotest.env.create_person(2, 2, { profession = 'worker' })
   worker:get_component('stonehearth:attributes'):set_attribute('hunger', hunger)
end

function food_tests.eat_food_on_ground()
   local food = autotest.env.create_entity(-2, -2, 'stonehearth:rabbit_jerky')
   autotest.util.succeed_when_destroyed(food)

   make_hungry_worker()
   autotest.util.fail_if_expired(60 * 1000, 'failed to eat food')
end

function food_tests.eat_food_in_container()
   local berry_basket = autotest.env.create_entity(-2, -2, 'stonehearth:berry_basket')
   berry_basket:get_component('item'):set_stacks(2)
   autotest.util.succeed_when_destroyed(berry_basket)

   make_hungry_worker()
   autotest.util.fail_if_expired(60 * 1000, 'failed to eat food')
end

function food_tests.eat_food_in_chair()
   local food = autotest.env.create_entity(-2, -2, 'stonehearth:rabbit_jerky')
   autotest.env.create_entity(6, 6, 'stonehearth:arch_backed_chair')

   autotest.util.succeed_when_destroyed(food)

   make_hungry_worker(81)
   autotest.util.fail_if_expired(300 * 1000, 'failed to eat food in chair')
end

return food_tests
