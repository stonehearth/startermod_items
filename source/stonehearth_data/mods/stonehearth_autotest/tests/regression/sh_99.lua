-- Trying to move a placed item that is in use, puts worker AI in bad state until the item can be moved.
-- https://bugs/browse/SH-99
--

local sh_99 = {}

function sh_99.verify_moving_placed_item_when_in_use(autotest) 
   local worker1 = autotest.env:create_person(4, 0, { job = 'worker' })
   local bed = autotest.env:create_entity(0, 10, 'stonehearth:furniture:comfy_bed', { force_iconic = false })
   local tree = autotest.env:create_entity(10, -10, 'stonehearth:large_oak_tree')

   -- Remember the current game speed.  We need to turn it down to 1 during the timing critical part of
   -- the test
   local current_speed
   _radiant.call('radiant:game:get_game_speed')
               :done(function(o)
                     current_speed = o.game_speed                     
                  end)

   -- move the calendar forward to trigger sleep
   stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })

   radiant.events.listen(worker1, 'stonehearth:sleep_in_bed', function()
         -- create the worker here so he doesn't participate in bed time
         local worker2 = autotest.env:create_person(6, 0, { job = 'worker' })
         autotest.ui:push_unitframe_command_button(bed, 'move_item')
         autotest.ui:click_terrain(10, 10)

         autotest.ui:click_dom_element('#startMenu #harvest_menu')
         autotest.ui:set_next_designation_region(10, -10, 1, 1) 

         -- slow down now so there's time for the ai to freak out during the chop animation
         _radiant.call('radiant:game:set_game_speed', 1.0)
      end)

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
         if e.entity:get_uri() == 'stonehearth:oak_log' then
            local constants = stonehearth.calendar:get_constants()
            stonehearth.calendar:set_time_unit_test_only({ hour = constants.start.hour, minute = constants.start.minute })
            autotest:log('restoring game speed of %.1f', current_speed)
            _radiant.call('radiant:game:set_game_speed', current_speed)
            autotest:success()
         end
      end)

   autotest:sleep(20000)
   autotest:fail('task failed to complete')
end

return sh_99
