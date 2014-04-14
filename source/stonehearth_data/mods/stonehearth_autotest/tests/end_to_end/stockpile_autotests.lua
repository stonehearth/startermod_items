local Point3f = _radiant.csg.Point3f

local stockpile_tests = {}

-- Sets the 'none' bit of the stockpile while the worker is getting ready to carry a log there.
-- This reproduces a nasty AI bug that we don't want showing up again.
function stockpile_tests.abort_stockpile_creation(autotest)
   local carry_change_count = 0
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')

   radiant.events.listen(worker, 'stonehearth:carry_block:carrying_changed', function (e)
      carry_change_count = carry_change_count + 1
      if carry_change_count == 2 then
         if autotest:script_did_error() then
            autotest:fail('Encountered error during execution.')
         else
            autotest:success()
         end
      end
      end)

   autotest:sleep(500)
   autotest.ui:click_dom_element('#startMenu #zone_menu')
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#startMenu div[hotkey="o"]')
   autotest.ui:sleep(500)
   autotest.ui:set_next_designation_region(4, 8, 1, 1)
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#stockpileWindow #none')
   autotest:sleep(10000)
   autotest:fail('Entity didn\'t drop its carry block.')
end

return stockpile_tests
