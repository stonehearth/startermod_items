local lrbt_util = require 'tests.longrunning.build.lrbt_util'
local Point3 = _radiant.csg.Point3

local construction_tests = {}

function construction_tests.simple_build(autotest)
   autotest.env:create_person(-6, 8, { job = 'worker' })
   autotest.env:create_person(-4, 3, { job = 'carpenter' })
   autotest.env:create_person(-10, 4, { job = 'worker' })
   autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:resources:wood:oak_log')
   local construction_complete = false

   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         if e.entity:get_uri() == 'stonehearth:build:prototypes:building' then
            local building = e.entity
            lrbt_util.succeed_when_buildings_finished(autotest, { building })
         end
   end)

   autotest:sleep(1000)
   
   autotest.ui:click_dom_element('#startMenu #build_menu')
   autotest.ui:click_dom_element('#startMenu #custom_building')
   autotest.ui:click_dom_element('#buildPalette area[tool=drawWallTool]')

   autotest.ui:click_terrain(-4, -2)
   autotest.ui:click_terrain( 2, -2)

   autotest.ui:click_dom_element('#showOverview')
   autotest.ui:click_dom_element('#startBuilding')
   autotest.ui:click_dom_element('#confirmBuilding')
   autotest.ui:click_dom_element('.stonehearthMenu .close')

   autotest:sleep(3*60*1000)
   autotest:fail('failed to construction')
end

return construction_tests
