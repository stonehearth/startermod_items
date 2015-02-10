local Point3 = _radiant.csg.Point3

local harvest_tests = {}

function harvest_tests.chop_tree(autotest)
   autotest.env:create_person(10, 10, { job = 'worker' })
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')

   autotest.util:succeed_when_destroyed(tree)
   
   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #harvest')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 
   
   autotest:sleep(20000)
   autotest:fail('worker failed to chop tree')
end


function harvest_tests.clear_test(autotest)
   autotest.env:create_person(10, 10, { job = 'worker' })
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')

   autotest.util:succeed_when_destroyed(tree)
   
   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #clear_item')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 
   
   --must click yes, confirm
   autotest.ui:click_dom_element('#confirmClear')


   autotest:sleep(20000)
   autotest:fail('worker failed to clear tree')
end

--test that the most recent command (here, harvest) command supercedes the previous command (clear)
function harvest_tests.supercede_command(autotest)   
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')
   
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:resources:wood:oak_log' then 
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #clear_item')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 
   autotest.ui:click_dom_element('#confirmClear')

   
   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #harvest')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 

   autotest:sleep(3000)

   autotest.env:create_person(20, 20, { job = 'worker' })
   
   autotest:sleep(20000)
   autotest:fail('worker failed to clear tree')
end


--test that cancel task works
---[[
function harvest_tests.cancel_task(autotest)   
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')
   
   local event = radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:resources:wood:oak_log' then 
         autotest:fail()
         return radiant.events.UNLISTEN
      end
   end)

   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #harvest')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 

   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:click_dom_element('#startMenu #cancel_task')
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 
   
   autotest:sleep(3000)
   autotest.env:create_person(10, 10, { job = 'worker' })

   autotest:sleep(3000)
   event:destroy()
   event = nil
   autotest:success('undo_task works')
end
--]]

return harvest_tests
