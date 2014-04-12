local Point3f = _radiant.csg.Point3f

local construction_tests = {}

function construction_tests.simple_build(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')
   local construction_complete = false

   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         if e.entity:get_uri() == 'stonehearth:scaffolding' then
            local scaffolding = e.entity
            radiant.events.listen(scaffolding, 'stonehearth:construction:finished_changed', function ()
                  -- scaffolding should be destroyed automagically.
                  --radiant.entities.destroy_entity(blueprint)

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

   -- blueprint is done!  wait for the scaffolding to be torn down before reporting success.
   -- the scaffolding will be "done" when the scaffolding blueprint region matches the
   -- scaffolding project.  since the original blueprint is done, the scaffolding region
   -- should now be empty
   radiant.events.listen(autotest.env:get_town(), 'stonehearth:blueprint_added', function (e)
         local blueprint = e.blueprint
         radiant.events.listen(blueprint, 'stonehearth:construction:finished_changed', function ()
               local finished = blueprint:get_component('stonehearth:construction_progress'):get_finished()
               if finished then
                  construction_complete = true
                  -- unlisten from notifications about the blueprint.
                  return radiant.events.UNLISTEN
               end
         end)
   end)
   
   
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#startMenu div[hotkey="l"]')

   autotest.ui:sleep(500)
   autotest.ui:click_terrain(7, 4)
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(5, 4)

   autotest:sleep(5*60*1000)
   autotest:fail('failed to construction')
end

return construction_tests
