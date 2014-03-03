local harvest_tests = {}

function harvest_tests.chop_tree()
   autotest.env.create_person(10, 10, { profession = 'worker' })
   local tree = autotest.env.create_entity(-2, -2, 'stonehearth:medium_oak_tree')

   autotest.ui.push_unitframe_command_button(tree, 'chop')
   
   autotest.sleep(20000000)
   autotest.fail('worker failed to chop tree')
end

return harvest_tests
