local immigration_tests = {}

function immigration_tests.get_one_citizen(autotest)
   autotest.env:create_person(2, 2, { job = 'worker' })

   local num_corn = 0
   local required_corn = 430

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if e.entity:get_uri() == 'stonehearth:food:corn:corn_basket' then
         num_corn = num_corn + 1
         if (num_corn >= required_corn) then
            return radiant.events.UNLISTEN
         end
      end
   end)

   --Have enough supplies to pass the criteria
   --50 food, > 4.0 morale, 100 net worth
   local cluster = autotest.env:create_entity_cluster(0, 0, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(0, 0, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(0, 10, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(0, 10, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(-10, 10, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(-10, 10, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(10, 10, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(10, 10, {size = {x=10, y=10}})

   -- Wait for corn spawn
   while num_corn < required_corn do
      autotest:sleep(1000)
   end

   -- Wait for score update
   local score_updated = false
   radiant.events.listen_once(radiant, 'stonehearth:score_updated', function (e)
      score_updated = true
      stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:immigration')
   end)

   -- Wait for bulletin
   radiant.events.listen(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', function(e)
      --Click the daily update button, click the scenario
      autotest.ui:click_dom_element('#popup')
      return radiant.events.UNLISTEN
   end)

   while not score_updated do
      autotest:sleep(1*1000)
   end

   autotest:sleep(5*1000)
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