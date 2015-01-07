local Point3 = _radiant.csg.Point3

local construction_tests = {}

function construction_tests.simple_build(autotest)
   autotest.env:create_person(-6, 8, { job = 'worker' })
   autotest.env:create_person(-4, 3, { job = 'carpenter' })
   autotest.env:create_person(-10, 4, { job = 'worker' })
   autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:resources:wood:oak_log')
   local construction_complete = false

   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         if e.entity:get_uri() == 'stonehearth:entities:building' then
            local building = e.entity
            radiant.events.listen(building, 'stonehearth:construction:finished_changed', function ()
                  local finished = building:get_component('stonehearth:construction_progress'):get_finished()
                  if finished then
                     construction_complete = true
                     -- unlisten from notifications about the blueprint.  The listen code above
                     -- will now wait for the scaffolding to be torn down.
                     return radiant.events.UNLISTEN
                  end
            end)
         end
         if e.entity:get_uri() == 'stonehearth:scaffolding' then
            local scaffolding = e.entity
            radiant.events.listen(scaffolding, 'stonehearth:construction:finished_changed', function ()
                  if construction_complete then
                     local finished = scaffolding:get_component('stonehearth:construction_progress'):get_finished()
                     local rgn = scaffolding:get_component('destination'):get_region()
                     if finished and rgn:get():get_area() == 0 then
                        autotest:success()
                        return radiant.events.UNLISTEN
                     end
                  end
            end)
            return radiant.events.UNLISTEN
         end
   end)

   autotest:sleep(1000)
   
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu img[hotkey="b"]')
   autotest.ui:click_dom_element('#customBuildingButton')
   autotest.ui:click_dom_element('#drawWallTool')

   autotest.ui:click_terrain(2, -2)
   autotest.ui:click_terrain(-4, -2)

   autotest.ui:click_dom_element('#showOverview')
   autotest.ui:click_dom_element('#startBuilding')
   autotest.ui:click_dom_element('#confirmBuilding')
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(3*60*1000)
   autotest:fail('failed to construction')
end

return construction_tests
