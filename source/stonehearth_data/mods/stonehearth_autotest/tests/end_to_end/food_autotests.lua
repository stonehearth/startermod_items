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

--Tests that people will eat till all the food is gone
function food_tests.group_eat_food()
   --Debugging opportunity: when this number is > 1, threading/task errors!
   local num_people = 2

   --Create n hungry people
   for i=1,num_people do 
      local p = autotest.env.create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })
   end

   --Each person will need 2 servings of tester_servings to be full,
   --and when we're all gone, we should return success
   local baskets = {}

   for i=1, num_people do 
      local b = autotest.env.create_entity(0, 0, 'stonehearth:tester_basket')
      table.insert(baskets, b)
   end

   local baskets_finished = 0
   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function()
      local all_destroyed = true
      for i, basket in ipairs(baskets) do
         if basket:is_valid() then
            all_destroyed = false
         end
      end
      if all_destroyed then
         autotest.success()
      end
   end)      

   --Adjust up?
   autotest.util.fail_if_expired(500 * 1000, 'failed to eat food in chair')
end

--[[
--Tests that people will eat till they're sated
--Doesn't work, due to the kill-thread bug mentioned below. Re-enable once
--that's fixed.
function food_tests.group_eat_till_full()
   local num_people = 10

   --Create 10 hungry people
   local worker_table = {}
   for i=1,num_people do 
      local p = autotest.env.create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })
      table.insert(worker_table, p)
   end

   --Each person will need 2 servings of tester_servings to be full,
   --so we need 20 tester baskets
   for i=1, num_people do 
      autotest.env.create_entity(0, 0, 'stonehearth:tester_basket')
   end

   --We're done if everyone's satisfaction is at 100
   local people_finished = 0
    for i, person in ipairs(worker_table) do

      --When we become satiated, we kill the eat task, which kills the whole thread
      --As a result, this listener is never called
      autotest.util.call_if_attribute_above(person, 'calories', 70, function()
         people_finished = people_finished + 1
         if people_finished == num_people then
            autotest.success()
         end
      end)
    end

   --Adjust up?
   autotest.util.fail_if_expired(9000 * 1000, 'failed to eat food in chair')
end
--]]

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
