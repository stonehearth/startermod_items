local promote_tests = {}

-- call to promote a unit.  the cb will be called the moment the promotion
-- happens (probably mid-air)
local function promote(autotest, worker, talisman, cb)
   autotest.ui:push_unitframe_command_button(worker, 'promote_to_job')
   autotest.ui:click_dom_element('#stonehearth\\:jobs\\:carpenter')
   autotest.ui:click_dom_element('#approveStamper')
   radiant.events.listen(worker, 'stonehearth:job_changed', function (e)
         cb(e)
         return radiant.events.UNLISTEN
      end)
end

function promote_tests.promote_to_carpenter(autotest)
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
   local saw = autotest.env:create_entity(1, 1, 'stonehearth:carpenter:saw_talisman')
  
   autotest:sleep(500)
   
   promote(autotest, worker, saw, function(e)
         autotest:success()         
      end)

   autotest:sleep(2000 * 1000)
   autotest:fail('failed to promote')
   
end

--[[
--This works when it's the only test in the file. Cleanup issue?
function promote_tests.promote_to_trapper(autotest)
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })
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
