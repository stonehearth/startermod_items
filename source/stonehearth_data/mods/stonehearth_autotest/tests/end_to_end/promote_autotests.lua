local promote_tests = {}

function promote_tests.promote_to_carpenter()
   local worker = autotest.env.create_person(2, 2, { profession = 'worker' })
   local saw = autotest.env.create_entity(-2, -2, 'stonehearth:carpenter:saw')

   radiant.events.listen(worker, 'stonehearth:profession_changed', function()
         autotest.success()
      end)

   autotest.ui.push_unitframe_command_button(saw, 'promote_to_profession')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#promoteScroll #chooseButton')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#peoplePicker #list #choice')
   autotest.ui.sleep(1000)
   autotest.ui.click_dom_element('#promoteScroll #approveStamper')

   autotest.sleep(2000 * 1000)
   autotest.fail('failed to promote')
end

return promote_tests
