local Point3f = _radiant.csg.Point3f

local harvest_tests = {}

function harvest_tests.chop_tree(autotest)
   autotest.env:create_person(10, 10, { profession = 'worker' })
   local tree = autotest.env:create_entity(1, 1, 'stonehearth:small_oak_tree')

   autotest.util:succeed_when_destroyed(tree)
   
   autotest.ui:sleep(500)
   autotest.ui:click_dom_element('#startMenu #harvest_menu')
   autotest.ui:sleep(500)
   autotest.ui:set_next_designation_region(-4, -4, 12, 12) 
   
   autotest:sleep(20000)
   autotest:fail('worker failed to chop tree')
end

return harvest_tests
