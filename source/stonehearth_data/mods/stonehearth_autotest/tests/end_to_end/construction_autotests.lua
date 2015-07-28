local lrbt_util = require 'tests.longrunning.build.lrbt_util'
local Point3 = _radiant.csg.Point3

local construction_tests = {}

function construction_tests.embedded(autotest)
   autotest.env:add_cube_to_terrain(-4, -4, 20, 2, 10, 200)
   autotest.env:create_person(-6, 8, { job = 'worker' })
   autotest.env:create_person(-4, 3, { job = 'carpenter' })
   autotest.env:create_person(-10, 4, { job = 'worker' })
   autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:resources:wood:oak_log')
   local construction_complete = false
   local num_complete = 0
   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         if e.entity:get_uri() == 'stonehearth:build:prototypes:building' then
            local building = e.entity

            lrbt_util.cb_when_buildings_finished({ building }, function()
                  num_complete = num_complete + 1
                  if num_complete == 2 then
                     autotest:success()
                  end
               end)
         end
   end)

   autotest:sleep(1000)
   
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu #custom_building')
   autotest.ui:click_dom_element('#buildPalette area[tool=drawWallTool]')

   -- Build a wall from left to right.
   autotest.ui:click_terrain(-4, -2)
   autotest.ui:click_terrain( 0, -2)

   autotest.ui:click_dom_element('#showOverview')
   autotest.ui:click_dom_element('#startBuilding')
   autotest.ui:click_dom_element('#confirmBuilding')
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu #custom_building')
   autotest.ui:click_dom_element('#buildPalette area[tool=drawWallTool]')

   -- Build a wall from right to left.  (Doing both directions tests the
   -- normal preference code.)
   autotest.ui:click_terrain(6, -2)
   autotest.ui:click_terrain(3, -2)

   autotest.ui:click_dom_element('#showOverview')
   autotest.ui:click_dom_element('#startBuilding')
   autotest.ui:click_dom_element('#confirmBuilding')
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(3*60*1000)
   autotest:fail('failed to build')
end

return construction_tests
