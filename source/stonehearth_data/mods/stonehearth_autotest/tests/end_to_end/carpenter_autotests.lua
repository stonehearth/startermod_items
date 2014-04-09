local Point3f = _radiant.csg.Point3f

local carpenter_tests = {}

function carpenter_tests.place_workshop(autotest)
   local carpenter = autotest.env:create_person(2, 2, { profession = 'carpenter' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')


   -- when creating the workshop, be sure to send the size and position of
   -- the outbox before selecting the workshop location.  otherwise, the
   -- ui and the server will race to see if it gets there in time!
   autotest.ui:push_unitframe_command_button(carpenter, 'build_workshop')
   autotest.ui:sleep(500)
   autotest.ui:click_terrain(4, 4)
   autotest.ui:sleep(500)
   autotest.ui:set_next_designation_region(4, 8, 4, 4) 

   local workshop
   radiant.events.listen(carpenter, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()
         autotest:resume()
      end)
   autotest:suspend()
   
   autotest.ui:push_unitframe_command_button(workshop, 'show_workshop')
   autotest.ui:sleep(1000)
   autotest.ui:click_dom_element('#craftWindow #recipeList a[recipe_name="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')

   local outbox = workshop:get_component('stonehearth:workshop'):get_outbox()
   radiant.events.listen(outbox, 'stonehearth:item_added', function(e)
         local name = e.item:get_component('unit_info'):get_display_name() 
         if name == 'Table for One' then
            autotest:success()
         else
            autotest:fail('expected Table for One and got "%s"', name)
         end
      end)

   autotest:sleep(120 * 10000)
   autotest:fail('failed to carpenter')
end

return carpenter_tests
