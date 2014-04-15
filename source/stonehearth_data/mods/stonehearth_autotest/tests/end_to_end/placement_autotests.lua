local placement_autotests = {}

--TODO: discover why the trace doesn't fire on the second of the 2 functions

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


--Move an existing item to a new location
--Pickup a proxy item and move it to a new location
--[[
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

return placement_autotests