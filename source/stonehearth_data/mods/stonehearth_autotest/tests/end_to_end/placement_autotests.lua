local placement_autotests = {}

---[[
--Pickup a proxy item and move it to a new location
function placement_autotests.place_one_proxy_item(autotest)
   autotest.env:create_person(10, 10, { profession = 'worker' })
   local bed_proxy = autotest.env:create_entity(0, 0, 'stonehearth:comfy_bed_proxy')

   --If the big bed moves to the target location, we win!
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      local target_entity = e.entity
      if target_entity:get_uri() == 'stonehearth:comfy_bed' then
         local trace = radiant.entities.trace_location(target_entity, 'sh placement autotest')
            :on_changed(function()
                  local location = radiant.entities.get_world_grid_location(target_entity)
                  if location.x == 6 and location.z == 6 then
                     autotest:success()
                     return radiant.events.UNLISTEN
                  end
               end)
      end
   end)
   
   autotest.ui:sleep(500)
   autotest.ui:push_unitframe_command_button(bed_proxy, 'place_item')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(6, 6)

   autotest:sleep(20000)
   autotest:fail('worker failed to place the bed')
end
--]]

--Move an existing item to a new location
---[[
function placement_autotests.move_one_proxy_item(autotest)
   autotest.env:create_person(10, 10, { profession = 'worker' })
   local bed = autotest.env:create_entity(0, 0, 'stonehearth:comfy_bed')

   local trace = radiant.entities.trace_location(bed, 'sh placement autotest')
            :on_changed(function()
                  local location = radiant.entities.get_world_grid_location(bed)
                  if location.x == 6 and location.z == 6 then
                     autotest:success()
                     return radiant.events.UNLISTEN
                  end
               end)
   
   autotest.ui:sleep(500)
   autotest.ui:push_unitframe_command_button(bed, 'move_item')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(6, 6)

   autotest:sleep(20000)
   autotest:fail('worker failed to move the bed')
end
--]]

--Test that 2 workers can compete to move the bed to a target location
--Starting from a proxy, place it and then move it several times
function placement_autotests.two_place_multiple_times(autotest)
   autotest.env:create_person(10, 10, { profession = 'worker' })
   autotest.env:create_person(-10, -10, { profession = 'worker' })

   local bed_proxy = autotest.env:create_entity(0, 0, 'stonehearth:comfy_bed_proxy')

   --If the big bed moves to the target location, we win!
   local big_bed
   local trace
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:comfy_bed' then
         big_bed = e.entity
         trace = radiant.entities.trace_location(big_bed, 'sh placement autotest')
            :on_changed(function()
                  local location = radiant.entities.get_world_grid_location(big_bed)
                  -- Check to see if the mob has a parent, meaning we're in the world, and not being
                  -- removed.  The trace fires when being removed, AND the location is set to 0,0,0,
                  -- which we want to ignore.
                  if big_bed:get_component('mob'):get_parent() ~= nil then
                     if location.x == 8 and location.z == 8 then
                        trace:destroy()
                        autotest:success()
                        return radiant.events.UNLISTEN
                     else
                        autotest.ui:push_unitframe_command_button(big_bed, 'move_item')
                        autotest.ui:sleep(500)
                        autotest.ui:click_terrain(location.x + 2, location.z + 2)
                     end
                  end
               end)
      end
   end)
   
   autotest.ui:sleep(500)
   autotest.ui:push_unitframe_command_button(bed_proxy, 'place_item')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(2, 2)

   autotest:sleep(60000)
   autotest:fail('worker failed to move the bed')
end

return placement_autotests