local rng = _radiant.csg.get_default_rng()
local redmine_320 = {}

NUM_BEDS = 2

function redmine_320.verify_place_multiple_times(autotest)
   local worker = autotest.env:create_person(4, 12, { job = 'worker', attributes = { mind = 6, body = 6, spirit = 6 } })
   local num_beds_placed = 0
   -- Create some beds
   local beds = {}

   local iconic_seed = rng:get_int(0, 1)

   for i=1, NUM_BEDS do
      local iconic = ((iconic_seed + i) % 2 == 0)
      local bed = autotest.env:create_entity(i * 4, 8, 'stonehearth:furniture:comfy_bed', { force_iconic = iconic })
      autotest.ui:push_unitframe_command_button(bed, 'move_item')

      local desired_location_x = i * 4
      local desired_location_z = 0

      autotest.ui:click_terrain(desired_location_x, desired_location_z)
      beds[i] = bed

      local trace
      trace = radiant.entities.trace_grid_location(bed, 'redmine 320 autotest')
         :on_changed(function()
            local location = radiant.entities.get_world_grid_location(bed)
            -- Check to see if the mob has a parent, meaning we're in the world, and not being
            -- removed.  The trace fires when being removed, AND the location is set to 0,0,0,
            -- which we want to ignore.
            if bed:get_component('mob'):get_parent() ~= nil then
               if location.x == desired_location_x and location.z == desired_location_z then
                  trace:destroy()
                  num_beds_placed = num_beds_placed + 1
                  if num_beds_placed == NUM_BEDS then
                     autotest:success()
                  end
                  return radiant.events.UNLISTEN
               end
            end
         end)
   end

   local trace
   trace = radiant.entities.trace_grid_location(worker, 'worker redmine 320 autotest')
      :on_changed(function()
         -- Check to see if the mob has a parent, meaning we're in the world, and not being
         -- removed.  The trace fires when being removed, AND the location is set to 0,0,0,
         -- which we want to ignore.
         local is_carrying = radiant.entities.get_carrying(worker)
         if is_carrying then
            radiant.entities.set_attribute(worker, 'sleepiness', stonehearth.constants.sleep.EXHAUSTION)
            trace:destroy()
         end
         return radiant.events.UNLISTEN
      end)

   autotest:sleep(30 * 1000)
   autotest:fail('workers failed to place all the beds')
end

return redmine_320