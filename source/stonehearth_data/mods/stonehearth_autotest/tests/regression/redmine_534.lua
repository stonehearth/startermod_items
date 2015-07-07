local rng = _radiant.csg.get_default_rng()
local entity_forms_lib = require 'stonehearth.lib.entity_forms.entity_forms_lib'

local redmine_534 = {}
NUM_TABLES = 7

function redmine_534.verify_craft_beds(autotest)
   local carpenter = autotest.env:create_person(4, 12, { job = 'carpenter', attributes = { mind = 6, body = 6, spirit = 6 } })
   local wood = autotest.env:create_entity_cluster(-3, -3, 3, 3, 'stonehearth:resources:wood:oak_log')

   local stockpile = autotest.env:create_stockpile(-10, -10, { size = { x = 5, y = 5 }})
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
   autotest.ui:click_dom_element('#craftWindow #recipeList div[title="Table for One"]')

   for i=2, NUM_TABLES do
      autotest.ui:click_dom_element('#craftWindow .inc')
   end

   autotest.ui:click_dom_element('#craftWindow #craftButton')
   autotest.ui:click_dom_element('#craftWindow #closeButton')

   local num_tables = 0
   radiant.events.listen(stockpile, 'stonehearth:stockpile:item_added', function(e)
      local item = e.item
      local root, iconic, ghost = entity_forms_lib.get_forms(item)
      if root then
         item = root   
      end
      if item:get_uri() == 'stonehearth:furniture:table_for_one' then
         num_tables = num_tables + 1

         if num_tables == NUM_TABLES then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
   end)

   autotest:sleep(45 * 1000)
   autotest:fail('failed to create 7 tables and place them in the stockpile')
end

return redmine_534