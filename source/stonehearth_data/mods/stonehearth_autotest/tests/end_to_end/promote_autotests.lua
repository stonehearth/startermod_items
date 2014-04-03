local promote_tests = {}

-- call to promote a unit.  the cb will be called the moment the promotion
-- happens (probably mid-air)
local function promote(worker, talisman, cb)
   autotest.ui.push_unitframe_command_button(talisman, 'promote_to_profession')
   autotest.ui.click_dom_element('#promoteScroll #chooseButton')
   autotest.ui.click_dom_element('#peoplePicker #list #choice')
   autotest.ui.click_dom_element('#promoteScroll #approveStamper')
   radiant.events.listen(worker, 'stonehearth:profession_changed', function (e)
         cb(e)
         return radiant.events.UNLISTEN
      end)
end

function promote_tests.promote_to_carpenter()
   local worker = autotest.env.create_person(2, 2, { profession = 'worker' })
   local saw = autotest.env.create_entity(-2, -2, 'stonehearth:carpenter:saw')

   -- will add the crafter component to a worker...
   promote(worker, saw, function(e)
         autotest.success()         
      end)

   autotest.sleep(2000 * 1000)
   autotest.fail('failed to promote')
end

return promote_tests
