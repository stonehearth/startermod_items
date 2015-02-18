-- Fallow fields are never replanted
-- https://bugs/browse/SH-107
--
-- Set a field's crop to fallow. A few minutes later, set a field's crop to a
-- new type of crop. The field is never planted with the new type of crop.
--
-- Build a field that is fallow to start. Click OK
-- Tell the field to change to tester crop. Click OK
-- Wait till we've created a tester basket.
--

local sh_107 = {} 

function sh_107.test_fallow_crop(autotest)
   local farmer = autotest.env:create_person(2, 2, { job = 'farmer' })

   --Add a fast-harvest test crop
   local session = {
      player_id = radiant.entities.get_player_id(farmer),
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:crops:tester_crop')

   local farm_entity = nil

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:farmer:field' then
            farm_entity = e.entity
         end
         if e.entity:get_uri() == 'stonehearth:tester_basket' then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)
   
   -- create a 1x1 farm
   autotest.ui:click_dom_element('#startMenu img[hotkey="f"]')
   autotest.ui:set_next_designation_region(4, 8, 1, 1)

   -- when the farm widget pops up, pick the fallow crop
   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('div[crop="fallow"]')

   -- click ok to close the dialog
   --autotest.ui:click_dom_element('#farmWindow .ok')

   autotest:sleep(9000)

   --autotest.ui:click_terrain(4, 8)
   autotest.ui:click_dom_element('#farmWindow #addCropLink')
   autotest.ui:click_dom_element('div[crop="stonehearth:crops:tester_crop"]')

   -- click ok to close the dialog
   autotest.ui:click_dom_element('#farmWindow .ok')

   --Close the zones window
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(60*1000)
   autotest:fail('failed to farm')
end

return sh_107