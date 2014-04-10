local Point3f = _radiant.csg.Point3f

local construction_tests = {}

function construction_tests.simple_build(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')

   radiant.events.listen(autotest.env:get_town(), 'stonehearth:building_added', function (e)
         local building = e.building
         radiant.events.listen(building, 'stonehearth:construction_finished', function (e)               
               if e.finished then
                  -- xxx: doesn't wait for the scaffolding to be finished, unfortunately
                  radiant.entities.destroy_entity(building)
                  autotest:success()
               end
            end)
      end)
   
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu div[hotkey="l"]')

   autotest.ui:click_terrain(7, 4)
   autotest.ui:click_terrain(4, 4)

   autotest:sleep(5*60*1000)
   autotest:fail('failed to construction')
end

return construction_tests
