local harvest_tests = {}

function harvest_tests.chop_tree()
   autotest.env.create_person(10, 10, { profession = 'worker' })
   local tree = autotest.env.create_entity(-2, -2, 'stonehearth:small_oak_tree')

   radiant.events.listen(radiant.events, 'stonehearth:entity:post_destroy', function()
         if not tree:is_valid() then
            autotest.success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest.ui.push_unitframe_command_button(tree, 'chop')
   
   autotest.sleep(20000)
   autotest.fail('worker failed to chop tree')
end

return harvest_tests
