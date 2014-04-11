local trapper_tests = {}

--[[
-- Tests whether harvesting a rabbit yields a pelt
-- The test hits the success state, but errors prevent it from existing
function trapper_tests.get_pelt(autotest)
   local trapper = autotest.env:create_person(8, 8, { profession = 'trapper' })
   local rabbit = autotest.env:create_entity(0, 0, 'stonehearth:rabbit')

   --Set up the listener for the trap and for the success condition (1 rabbit pelt)
   local trap
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:trapper:snare_trap' then
         trap = e.entity
         autotest:resume()
      elseif e.entity:get_uri() == 'stonehearth:rabbit_pelt' then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest.ui:push_unitframe_command_button(trapper, 'snare_trap')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(0, 0)
   autotest.ui:sleep(500)

   --Wait for the trap to appear
   autotest:suspend()

   --Ok, should have the trap at this point
   if not trap then
      autotest:fail('failed to trap the critter. Did it move?')
   end

   autotest.ui:push_unitframe_command_button(trap, 'harvest_trapped_beast')
   autotest:sleep(100 * 10000)

   autotest:fail('failed to get a rabbit pelt')
end
--]]

---[[
--Test the pet follow behavior
--This test works in isolation, but does not work when the other tests in the file are on
function trapper_tests.follow_obsession(autotest)
   --Make a trapper and a pet
   local trapper = autotest.env:create_person(8, 8, { profession = 'trapper' })
   local rabbit = autotest.env:create_entity(0, 0, 'stonehearth:rabbit')
   local town = stonehearth.town:get_town(trapper)
   local equipment = rabbit:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:pet_collar')
   town:add_pet(rabbit)

   autotest.ui:sleep(1000)

   --Move the trapper; the pet should follow
   --Now, move the trapper 
   autotest.ui:push_unitframe_command_button(trapper, 'move_unit')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(-10, -10)
   autotest:sleep(10000)

   --Make sure the critter is within a certain radius of him
   local distance = radiant.entities.distance_between(trapper, rabbit)
   if distance < 3 then
      autotest:success()
   else
      autotest:fail('The critter isn not beside the trapper.')
   end
end
--]]

--[[
--Test whether the trapper's pets will follow him after they've been tamed
--Test does not yet work because the "struggle" compelled behavior is not 
--correctly removed from the rabbit
function trapper_tests.make_friends(autotest)
   local trapper = autotest.env:create_person(8, 8, { profession = 'trapper' })
   local rabbit = autotest.env:create_entity(0, 0, 'stonehearth:rabbit')

   local trap
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:trapper:snare_trap' then
         trap = e.entity
         autotest:resume()
         return radiant.events.UNLISTEN
      end
   end)

   autotest.ui:push_unitframe_command_button(trapper, 'snare_trap')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(0, 0)
   autotest.ui:sleep(500)

   --Wait for the trap to appear
   autotest:suspend()

   --Ok, should have the trap at this point
   if not trap then
      autotest:fail('failed to trap the critter. Did it move?')
   end

   autotest.ui:push_unitframe_command_button(trap, 'tame_trapped_beast')
   autotest:sleep(5 * 10000)

   --Now, move the trapper 
   autotest.ui:push_unitframe_command_button(trapper, 'move_unit')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(-10, -10)
   autotest:sleep(10000)

   --Make sure the critter is within a certain radius of him
   local distance = radiant.entities.distance_between(trapper, rabbit)
   if distance < 3 then
      autotest:success()
   else
      autotest:fail('The critter isn not beside the trapper.')
   end
end
--]]

return trapper_tests