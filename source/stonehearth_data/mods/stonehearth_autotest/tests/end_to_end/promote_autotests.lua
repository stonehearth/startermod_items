local promote_tests = {}

-- call to promote a unit.  the cb will be called the moment the promotion
-- happens (probably mid-air)
local function promote(autotest, worker, talisman, cb)
   autotest.ui:push_unitframe_command_button(talisman, 'promote_to_profession')
   autotest.ui:click_dom_element('#promoteScroll #chooseButton')
   autotest.ui:click_dom_element('#peoplePicker #list #choice')
   autotest.ui:click_dom_element('#promoteScroll #approveStamper')
   radiant.events.listen(worker, 'stonehearth:profession_changed', function (e)
         cb(e)
         return radiant.events.UNLISTEN
      end)
end

function promote_tests.promote_to_carpenter(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local saw = autotest.env:create_entity(-2, -2, 'stonehearth:carpenter:saw')

   autotest:sleep(1000)

   -- will add the crafter component to a worker...
   promote(autotest, worker, saw, function(e)
         autotest:success()         
      end)

   autotest:sleep(2000 * 1000)
   autotest:fail('failed to promote')
end

--[[
--This works when it's the only test in the file. Cleanup issue?
function promote_tests.promote_to_trapper(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local knife = autotest.env:create_entity(-2, -2, 'stonehearth:trapper:trapper_knife')

   -- will add the crafter component to a worker...
   promote(autotest, worker, knife, function(e)
         autotest:success()         
      end)

   autotest:sleep(2000 * 1000)
   autotest:fail('failed to promote')
end
--]]

return promote_tests
