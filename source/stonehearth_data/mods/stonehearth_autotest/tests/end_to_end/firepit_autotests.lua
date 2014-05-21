local firepit_autotests = {}

---[[
--Basic firepit test:
--do we light a fire at sunset?
--do we admire it?
--do we extinguish it at sunrise?
function firepit_autotests.light_fire(autotest)
   local worker = autotest.env:create_person(10, 10, { profession = 'worker' })
   local firepit = autotest.env:create_entity(0, 0, 'stonehearth:firepit')
   local wood = autotest.env:create_entity(-5, -5, 'stonehearth:oak_log')
   radiant.entities.set_player_id(firepit, radiant.entities.get_player_id(worker))

   local is_lit_at_night = false
   local is_admiring_fire = false
   local is_quenched_at_dawn = false

   --autotest:sleep(10000)

   --Listen for the "lit" event. 
   radiant.events.listen(stonehearth.events, 'stonehearth:fire:lit', function(e)
      if e.entity:get_id() ~= firepit:get_id() then
         return
      end
      if e.lit then
         --The firepit was just lit
         is_lit_at_night = true

         --Make it midnight to reset the daily counters
         stonehearth.calendar:set_time_unit_test_only({ hour = 0 })
      else 
         --The fire was just extinguished
         if is_lit_at_night and is_admiring_fire then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
   end)

   --Check if we're admiring the fire
   radiant.events.listen(worker, 'stonehearth:admiring_fire', function(e)
      is_admiring_fire = true

      --Make it dawn
      stonehearth.calendar:set_time_unit_test_only({ hour = 6 })
      return radiant.events.UNLISTEN
   end)

   --Make it night time so they light the fire
   stonehearth.calendar:set_time_unit_test_only({ hour = 22 })

   autotest:sleep(200000)
   autotest:fail('worker failed to light or admire or quench the fire')
end
--]]

-- Advanced firepit test, including admiring after move. 
-- When we move a lit firepit, do people resume admiring afterwards?
function firepit_autotests.move_lit_fire(autotest)
   local worker = autotest.env:create_person(10, 10, { profession = 'worker' })
   local worker2 = autotest.env:create_person(8, 8, { profession = 'worker' })
   local firepit = autotest.env:create_entity(0, 0, 'stonehearth:firepit')
   local wood = autotest.env:create_entity(-5, -5, 'stonehearth:oak_log')
   local wood2 = autotest.env:create_entity(-6, -5, 'stonehearth:oak_log')
   radiant.entities.set_player_id(firepit, radiant.entities.get_player_id(worker))

   --Make it night time so they light the fire
   stonehearth.calendar:set_time_unit_test_only({ hour = 22 })

   --Once it's lit, move it
   radiant.events.listen(stonehearth.events, 'stonehearth:fire:lit', function(e)
      if not e.entity:is_valid() or e.entity:get_id() ~= firepit:get_id() then
         return
      end
      if e.lit then
         autotest.ui:push_unitframe_command_button(firepit, 'move_item')
         autotest.ui:click_terrain(10, 10)
      end
      return radiant.events.UNLISTEN
   end)

   --Once it's moved, test to see if we're admiring the fire. 
   local firepit_moved = false
   local trace = radiant.entities.trace_location(firepit, 'sh firepit autotest')
            :on_changed(function()
               local location = radiant.entities.get_world_grid_location(firepit)
               -- Check to see if the mob has a parent, meaning we're in the world, and not being
               -- removed.  The trace fires when being removed, AND the location is set to 0,0,0,
               -- which we want to ignore.
               if firepit:get_component('mob'):get_parent() ~= nil then
                  if location.x == 10 and location.z == 10 then
                     firepit_moved = true
                     
                     --Make sure it's still nighttime
                     return radiant.events.UNLISTEN
                  end
               end
            end)

   --If we're admiring the fire after the move, the test succeeded
   radiant.events.listen(worker2, 'stonehearth:admiring_fire', function(e)
      if firepit_moved then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(200000)
   autotest:fail('nobody found the fire after it was moved')
end

return firepit_autotests