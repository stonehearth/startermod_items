local farming_tests = {}

function farming_tests.grow_one_turnip()
   local worker = autotest.env.create_person(2, 2, { profession = 'farmer' })

   radiant.events.listen(radiant, 'stonehearth:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:turnip_basket' then
            autotest.success()
         end
      end)
   
   -- create a 2x2 farm
   autotest.ui.click_dom_element('#startMenu #build_menu')
   autotest.ui.click_dom_element('#startMenu div[hotkey="f"]')
   autotest.ui.sleep(500)
   autotest.ui.set_next_designation_region(4, 8, 1, 1)

   -- when the farm widget pops up, pick the turnip crop
   autotest.ui.sleep(500)
   autotest.ui.click_dom_element('#farmWindow #addCropLink')
   autotest.ui.sleep(500)
   autotest.ui.click_dom_element('#cropPalette img[crop="stonehearth:turnip_crop"]')
   autotest.ui.sleep(500)

   -- click ok to close the dialog
   autotest.ui.click_dom_element('#farmWindow .ok')

   autotest.sleep(500000*60*1000)
   autotest.fail('failed to farm')
end

return farming_tests
