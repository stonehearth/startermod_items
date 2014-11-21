-- Crafters sometimes leave items on workbench if there is no stockpile
-- https://bugs/browse/SH-82
--
-- ...and once something is on the bench, it will stay there forever.
-- Our guess was that the crafter is choosing a point near the bench to drop
-- the item, and happens to choose the location of the bench itself.
--

local sh_82 = {} 

function sh_82.verify_not_on_bench(autotest)
   local carpenter = autotest.env:create_person(2, 2, { job = 'carpenter' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:resources:wood:oak_log')

   autotest:sleep(1000)

   -- when creating the workshop, be sure to send the size and position of
   -- the outbox before selecting the workshop location.  otherwise, the
   -- ui and the server will race to see if it gets there in time!
   autotest.ui:push_unitframe_command_button(carpenter, 'build_workshop')
   autotest.ui:click_terrain(4, 4)

   local workshop
   radiant.events.listen(carpenter, 'stonehearth:crafter:workshop_changed', function (e)
         workshop = e.workshop:get_entity()
         autotest:resume()
         return radiant.events.UNLISTEN
      end)
   autotest:suspend()
   
   autotest.ui:push_unitframe_command_button(workshop, 'show_workshop')
   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')

   autotest:sleep(1000)
   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
   
   autotest:sleep(1000)
   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
   
   autotest:sleep(1000)
   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="Table for One"]')
   autotest.ui:click_dom_element('#craftWindow #craftButton')
   autotest.ui:click_dom_element('#craftWindow #closeButton')

   local num_created = {}
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:furniture:table_for_one' then 
         local table_proxy = e.entity:get_component('stonehearth:entity_forms')
                                 :get_iconic_entity()
         table.insert(num_created, table_proxy)
         if #num_created == 4 then
            --check that the first 3 items are at ground level
            for i, table_object in ipairs(num_created) do
               if i < 3 and radiant.entities.get_world_grid_location(table_object).y > radiant.entities.get_world_grid_location(carpenter).y then
                  autotest:fail('object is not on the ground')
               end
            end
            --If 1, 2, and 3 are on the ground, then the test is a success
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end

   end)

   autotest:sleep(120 * 10000)
   autotest:fail('failed to carpenter')
end

return sh_82