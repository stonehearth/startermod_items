local Point3f = _radiant.csg.Point3f

local stockpile_tests = {}

function stockpile_tests.abort_stockpile_creation(autotest)

   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')

   autotest:sleep(500)
   autotest.ui:click_dom_element('#startMenu #zone_menu')
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#startMenu div[hotkey="o"]')
   autotest.ui:sleep(500)
   autotest.ui:set_next_designation_region(4, 8, 1, 1)
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#stockpileWindow #none')
   autotest:sleep(3000)
   
   if autotest:script_did_error() then
      autotest:fail('Encountered error during execution.')
   else
      autotest:success()
   end
end

return stockpile_tests
