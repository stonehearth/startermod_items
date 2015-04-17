local immigration_tests = {}

function immigration_tests.get_one_citizen(autotest)
   autotest.env:create_person(2, 2, { job = 'worker' })

   --Have enough supplies to pass the criteria
   --50 food, > 4.0 morale, 100 net worth
   local cluster = autotest.env:create_entity_cluster(0, 0, 10, 10, 'stonehearth:food:berries:berry_basket')
   local stockpile = autotest.env:create_stockpile(0, 0, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(0, 10, 10, 10, 'stonehearth:food:berries:berry_basket')
   local stockpile = autotest.env:create_stockpile(0, 10, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(-10, 10, 10, 10, 'stonehearth:food:berries:berry_basket')
   local stockpile = autotest.env:create_stockpile(-10, 10, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(10, 10, 10, 10, 'stonehearth:food:berries:berry_basket')
   local stockpile = autotest.env:create_stockpile(10, 10, {size = {x=10, y=10}})


   autotest:sleep(10*1000)

   --Fire the scenario
   stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:immigration')

   --Click the daily update button, click the scenario
   autotest.ui:click_dom_element('#bulletin_manager') 
   autotest.ui:click_dom_element('#bulletinList div .info') 

   autotest:sleep(4*1000)

   --Click Accept
   autotest.ui:click_dom_element('#acceptButton') 

   --Verify that you got a new person
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if stonehearth.population:get_population_size('player_1') == 2 then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(60*1000)
   autotest:fail('failed to get person')
end

return immigration_tests