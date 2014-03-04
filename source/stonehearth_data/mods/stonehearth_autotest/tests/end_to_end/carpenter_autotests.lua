local carpenter_tests = {}

function carpenter_tests.place_workshop()
   local carpenter = autotest.env.create_person(2, 2, { profession = 'carpenter' })
   local wood = autotest.env.create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')


   autotest.ui.push_unitframe_command_button(carpenter, 'build_workshop')
   autotest.ui.sleep(1000)
   autotest.ui.click_terrain(8, 8)

   --[[
   radiant.events.listen(worker, 'stonehearth:profession_changed', function()
         autotest.success()
      end)

   autotest.ui.push_unitframe_command_button(saw, 'carpenter_to_profession')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#carpenterScroll #chooseButton')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#peoplePicker #list #choice')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#carpenterScroll #approveStamper')
   ]]

   autotest.sleep(2000 * 1000)
   autotest.fail('failed to carpenter')
end

return carpenter_tests
