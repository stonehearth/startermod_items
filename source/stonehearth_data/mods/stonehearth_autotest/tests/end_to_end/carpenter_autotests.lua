local Point3f = _radiant.csg.Point3f

local carpenter_tests = {}

---[[
function carpenter_tests.place_workshop(autotest)
   local carpenter = autotest.env:create_person(2, 2, { profession = 'carpenter' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')

   autotest:sleep(500)

   -- when creating the workshop, be sure to send the size and position of
   -- the outbox before selecting the workshop location.  otherwise, the
   -- ui and the server will race to see if it gets there in time!
   autotest.ui:push_unitframe_command_button(carpenter, 'build_workshop')
   autotest.ui:click_terrain(4, 4)
   --autotest.ui:set_next_designation_region(4, 8, 4, 4) 

   local workshop
   radiant.events.listen(carpenter, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()
         autotest:resume()
         return radiant.events.UNLISTEN
      end)
   autotest:suspend()
   
   autotest.ui:push_unitframe_command_button(workshop, 'show_workshop')
   autotest.ui:click_dom_element('#craftWindow #recipeList a[recipe_name="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
   autotest.ui:click_dom_element('#craftWindow #closeButton')

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:table_for_one' then 
         autotest:success()
         return radiant.events.UNLISTEN
      end

   end)

   autotest:sleep(120 * 10000)
   autotest:fail('failed to carpenter')
end
--]]

--TODO: write an autotest for maintain
function carpenter_tests.maintain_x(autotest)
   autotest.env:create_person(2, 2, { profession = 'worker' })
   local stockpile = autotest.env:create_stockpile(-10, -10, { size = { x = 5, y = 5 }})

   local carpenter = autotest.env:create_person(2, 2, { profession = 'carpenter' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')

   autotest:sleep(500)

   -- when creating the workshop, be sure to send the size and position of
   -- the outbox before selecting the workshop location.  otherwise, the
   -- ui and the server will race to see if it gets there in time!
   autotest.ui:push_unitframe_command_button(carpenter, 'build_workshop')
   autotest.ui:click_terrain(4, 4)
   --autotest.ui:set_next_designation_region(4, 8, 4, 4) 

   local workshop
   radiant.events.listen(carpenter, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()
         autotest:resume()
         return radiant.events.UNLISTEN
      end)
   autotest:suspend()

   autotest.ui:push_unitframe_command_button(workshop, 'show_workshop')
   autotest.ui:click_dom_element('#craftWindow #recipeList a[recipe_name="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #maintain')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
   autotest.ui:click_dom_element('#craftWindow #closeButton')

   local num_built = 0
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:table_for_one' then 
         num_built = num_built + 1
         if num_built == 1 then
         local table_proxy = e.entity:get_component('stonehearth:entity_forms')
                                 :get_iconic_entity()
         autotest.ui:push_unitframe_command_button(table_proxy, 'place_item')
            autotest.ui:click_terrain(2, 2)
         elseif num_built == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
   end)
   
   autotest:sleep(120 * 10000)
   autotest:fail('failed to mantain 1 wooden table')

end

return carpenter_tests
