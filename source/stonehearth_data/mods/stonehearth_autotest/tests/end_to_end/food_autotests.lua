local Point3f = _radiant.csg.Point3f

local food_tests = {}

local function create_cluster(n, fn)
   local r = math.max(math.floor(math.sqrt(n) / 2), 2)
   local x, y = -r, -r
   for i = 1,n do
      x = x + 1
      if x == r then
         y = y + 1
         x = -r
      end
      fn(x, y)
   end
end

function food_tests.eat_food_on_ground(autotest)
   local food = autotest.env:create_entity(-2, -2, 'stonehearth:rabbit_jerky')

   autotest.env:create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })

   autotest.util:succeed_when_destroyed(food)
   autotest.util:fail_if_expired(40 * 1000, 'failed to eat food')
end

function food_tests.eat_food_in_container(autotest)
   local berry_basket = autotest.env:create_entity(-2, -2, 'stonehearth:berry_basket')
   berry_basket:get_component('item'):set_stacks(2)

   autotest.env:create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0  }
      })
   autotest.util:succeed_when_destroyed(berry_basket)
   autotest.util:fail_if_expired(40 * 1000, 'failed to eat food')
end

--Tests that people will eat till all the food is gone
function food_tests.group_eat_food(autotest)
   local num_people = 5

   --Create n hungry people
   create_cluster(num_people, function(x, y)
         local p = autotest.env:create_person(x + 2, y + 2, {
            profession = 'worker',
            attributes = { calories = 0 }
         })
      end)

   --Each person will need 2 servings of tester_servings to be full,
   --and when we're all gone, we should return success
   local baskets = {}

create_cluster(num_people, function(x, y)
         local b = autotest.env:create_entity(x - 2, y - 2, 'stonehearth:tester_basket')
         table.insert(baskets, b)
      end)

   local baskets_finished = 0
   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function()
      local all_destroyed = true
      for i, basket in ipairs(baskets) do
         if basket:is_valid() then
            all_destroyed = false
         end
      end
      if all_destroyed then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)      

   --Adjust up?
   autotest.util:fail_if_expired(500 * 1000, 'failed to eat food in chair')
end

--Tests that people will eat till they're sated
function food_tests.group_eat_till_full(autotest)
   local num_people = 10

   --Create 10 hungry people
   local worker_table = {}
   create_cluster(num_people, function(x, y)
         local p = autotest.env:create_person(x+5, y+5, {
            profession = 'worker',
            attributes = { calories = 0 }
         })
         table.insert(worker_table, p)
      end)

   --Each person will need 2 servings of tester_servings to be full,
   --so we need 20 tester baskets
   create_cluster(num_people, function(x, y)
         autotest.env:create_entity(x-5, y-5, 'stonehearth:tester_basket')
      end)

   --We're done if everyone's satisfaction is at 100
   local people_finished = 0
    for i, person in ipairs(worker_table) do

      --When we become satiated, we kill the eat task, which kills the whole thread
      --As a result, this listener is never called
      autotest.util:call_if_attribute_above(person, 'calories', 70, function()
         people_finished = people_finished + 1
         if people_finished == num_people then
            autotest:success()            
         end
         return radiant.events.UNLISTEN
      end)
    end

   --Adjust up?
   autotest.util:fail_if_expired(9000 * 1000, 'failed to eat food in chair')
end

function food_tests.eat_food_in_chair(autotest)
   autotest.env:create_entity(6, 6, 'stonehearth:arch_backed_chair')

   local food = autotest.env:create_entity(-2, -2, 'stonehearth:rabbit_jerky')
   local p = autotest.env:create_person(2, 2, {
         profession = 'worker',
         attributes = { calories = 0 }
      })

   autotest.util:succeed_when_destroyed(food)
   autotest.util:fail_if_expired(40 * 1000, 'failed to eat food in chair')
end


return food_tests
