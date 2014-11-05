
local sh_99 = {}

-- fails reliably at game speed 1, but not at higher game speeds
function sh_99.verify_moving_placed_item_when_in_use(autotest) 
   local worker1 = autotest.env:create_person(4, 0, { job = 'worker' })
   local bed = autotest.env:create_entity(0, 10, 'stonehearth:furniture:comfy_bed', { force_iconic = false })
   local tree = autotest.env:create_entity(10, -10, 'stonehearth:large_oak_tree')

   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   radiant.events.listen(worker1, 'stonehearth:sleep_in_bed', function()
         -- create the worker here so he doesn't participate in bed time
         local worker2 = autotest.env:create_person(6, 0, { job = 'worker' })
         autotest.ui:push_unitframe_command_button(bed, 'move_item')
         autotest.ui:click_terrain(10, 10)

         autotest.ui:click_dom_element('#startMenu #harvest_menu')
         autotest.ui:set_next_designation_region(10, -10, 1, 1) 
      end)

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:oak_log' then
            local constants = stonehearth.calendar:get_constants()
            stonehearth.calendar:set_time_unit_test_only({ hour = constants.start.hour, minute = constants.start.minute })
            autotest:success()
         end
      end)

   autotest:sleep(20000)
   autotest:fail('task failed to complete')
end

return sh_99
