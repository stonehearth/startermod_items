local farming_tests = {}

function farming_tests.grow_one_turnip(autotest)
   local farmer = autotest.env:create_person(2, 2, { profession = 'farmer' })

   --Add a fast-harvest test crop
   local session = {
      player_id = radiant.entities.get_player_id(farmer),
      kingdom = radiant.entities.get_kingdom(farmer)
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:tester_crop')


   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:tester_basket' then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)
   
   -- create a 2x2 farm
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu div[hotkey="f"]')
   autotest.ui:set_next_designation_region(4, 8, 1, 1)

   -- when the farm widget pops up, pick the turnip crop
   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('#cropPalette img[crop="stonehearth:tester_crop"]')

   -- click ok to close the dialog
   autotest.ui:click_dom_element('#farmWindow .ok')

   --Close the zones window
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(60*1000)
   autotest:fail('failed to farm')
end

return farming_tests
