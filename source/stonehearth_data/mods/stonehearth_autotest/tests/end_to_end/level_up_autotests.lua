local level_up_autotests = {}

function level_up_autotests.level_up_farmer(autotest)
   local farmer = autotest.env:create_person(2, 2, { job = 'farmer' })

   --Add a fast-harvest test crop
   local session = {
      player_id = radiant.entities.get_player_id(farmer),
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_crop')

   --The farmers should level up on producing 1 tester crop so should also get the buff
   --confirm the buff is present
   radiant.events.listen(farmer, 'stonehearth:level_up', function (e)
         if farmer:get_component('stonehearth:buffs'):has_buff('stonehearth:buffs:farmer:speed_1') then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   -- create a 1x1 farm
   autotest.ui:click_dom_element('#startMenu #create_farm')
   autotest.ui:set_next_designation_region(4, 8, 1, 1)

   -- when the farm widget pops up, pick the turnip crop
   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('[crop="stonehearth:crops:tester_crop"]')

   -- click ok to close the dialog
   autotest.ui:click_dom_element('#farmWindow .ok')

   --Close the zones window
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(60*1000)
   autotest:fail('failed to get level up buff')
end

return level_up_autotests