local Point3f = _radiant.csg.Point3f

local food_tests = {}

function food_tests.eat_food_on_ground()
   local food = autotest.env.create_entity(-2, -2, 'stonehearth:rabbit_jerky')

   autotest.env.create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })

   autotest.util.succeed_when_destroyed(food)
   autotest.util.fail_if_expired(40 * 1000, 'failed to eat food')
end

function food_tests.eat_food_in_container()
   local berry_basket = autotest.env.create_entity(-2, -2, 'stonehearth:berry_basket')
   berry_basket:get_component('item'):set_stacks(2)

   autotest.env.create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0  }
      })
   autotest.util.succeed_when_destroyed(berry_basket)
   autotest.util.fail_if_expired(40 * 1000, 'failed to eat food')
end

function food_tests.eat_food_in_chair()
   autotest.env.create_entity(6, 6, 'stonehearth:arch_backed_chair')

   local food = autotest.env.create_entity(-2, -2, 'stonehearth:rabbit_jerky')
   local p = autotest.env.create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })

   autotest.util.succeed_when_destroyed(food)
   autotest.util.succeed_if_attribute_above(p, 'calories', 0)
   autotest.util.fail_if_expired(40 * 1000, 'failed to eat food in chair')
end

return food_tests
