local harvest_tests = {}

function harvest_tests.chop_tree(autotest)
   autotest.env:create_person(10, 10, { profession = 'worker' })
   local tree = autotest.env:create_entity(-2, -2, 'stonehearth:small_oak_tree')

   autotest.util:succeed_when_destroyed(tree)
   autotest.ui:push_unitframe_command_button(tree, 'chop')
   
   autotest:sleep(20000)
   autotest:fail('worker failed to chop tree')
end

return harvest_tests
