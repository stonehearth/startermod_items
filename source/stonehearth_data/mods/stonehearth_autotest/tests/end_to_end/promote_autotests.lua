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

-- Start as a farmer, level up, promote to carpenter, check the exp to next level is correct
function promote_tests.progression_test(autotest)
   local civ = autotest.env:create_person(2, 2, { job = 'farmer' })
   local saw = autotest.env:create_entity(1, 1, 'stonehearth:carpenter:saw_talisman')

   --Add a fast-harvest test crop
   local session = {
      player_id = radiant.entities.get_player_id(civ),
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:tester_crop')

   --Listen for level up
   radiant.events.listen(civ, 'stonehearth:level_up', function(e)
      --Test that we have the right perk
      --If the this entity has the right perk, spawn more than one
      local job_component = civ:get_component('stonehearth:job')
      if job_component and job_component:curr_job_has_perk('farmer_speed_up_1') then
         --change jobs to carpenter

         promote(autotest, civ, saw, function(e)
            --test the progression to next level, should be 400
            if (job_component:get_xp_to_next_lv() == 400) then
               autotest:success()         
            else 
               autotest:fail('xp on level up isnt correct amount')
            end
         end)
         
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(1000)

   --The farming part
   -- create a 1x1 farm
   autotest.ui:click_dom_element('#startMenu div[hotkey="f"]')
   autotest.ui:set_next_designation_region(4, 8, 1, 1)

   autotest:sleep(100)

   -- when the farm widget pops up, pick the turnip crop
   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('.itemPalette div[crop="stonehearth:tester_crop"]')

   -- click ok to close the dialog
   autotest.ui:click_dom_element('#farmWindow .ok')

   --Close the zones window
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(60*1000)
   autotest:fail('failed to farm')

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
