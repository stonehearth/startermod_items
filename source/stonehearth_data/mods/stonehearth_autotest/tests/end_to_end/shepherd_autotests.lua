local shepherd_autotests = {}

---[[
function shepherd_autotests.collect_two_sheep(autotest)
   --put down one shepherd and one sheep
   local shepherd = autotest.env:create_person(2, 2, { job = 'shepherd' })
   autotest.env:create_entity(-3, -6, 'stonehearth:sheep')

   --Wait for her to return 3 animals to the pasture (2 from first time, 1 from regather)
   local num_animals_led = 0
   radiant.events.listen(shepherd, 'stonehearth:leave_animal_in_pasture', function(e)
      num_animals_led = num_animals_led + 1
      if num_animals_led == 3 then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(1000)

   --Make a pasture
   autotest.ui:click_dom_element('#startMenu #create_pasture')
   autotest.ui:set_next_designation_region(4, 8, 10, 10)

   autotest:sleep(100*1000)
   autotest:fail('failed to herd sheep')
end
--]]

---[[
function shepherd_autotests.demote(autotest)
   local shepherd = autotest.env:create_person(2, 2, { job = 'shepherd' })
   autotest.env:create_entity(-3, -6, 'stonehearth:sheep')
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')
   autotest.util:succeed_when_destroyed(tree)

   autotest:sleep(1000)

   --Set the harvest region
   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #harvest')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 

   autotest:sleep(1000)

   --Make a pasture
   autotest.ui:click_dom_element('#startMenu #create_pasture')
   autotest.ui:set_next_designation_region(4, 8, 10, 10)

   autotest:sleep(3000)

   --Demote the shepherd
   autotest.ui:push_unitframe_command_button(shepherd, 'promote_to_job')
   autotest.ui:click_dom_element('#stonehearth\\:jobs\\:worker')
   autotest.ui:click_dom_element('#approveStamper')
   
   autotest:sleep(100*1000)
   autotest:fail('failed to demote shepherd')
end
--]]

---[[
function shepherd_autotests.harvest_wool(autotest)
   --put down one shepherd and one sheep
   local shepherd = autotest.env:create_person(2, 2, { job = 'shepherd' })
   autotest.env:create_entity(0, 0, 'stonehearth:sheep')

   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #harvest')
   autotest.ui:set_next_designation_region(-10, -10, 20, 20) 
   
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:resources:fiber:wool_bundle' then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(60*1000)
   autotest:fail('failed to collect wool')
end
--]]

---[[
function shepherd_autotests.harvest_meat(autotest)
   local shepherd = autotest.env:create_person(2, 2, { job = 'shepherd' })
   local sheep = autotest.env:create_entity(0, 0, 'stonehearth:sheep')

   autotest:sleep(1000)

   --Make a pasture
   autotest.ui:click_dom_element('#startMenu #create_pasture')
   autotest.ui:set_next_designation_region(4, 8, 10, 10)

   --Wait for her to tame the critter, then harvest it
   radiant.events.listen(shepherd, 'stonehearth:tame_animal', function(e)
      autotest.ui:push_unitframe_command_button(sheep, 'slaughter')
      return radiant.events.UNLISTEN
   end)

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:sheep_jerky' then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(100*1000)
   autotest:fail('failed to harvest meat')

end
--]]

return shepherd_autotests