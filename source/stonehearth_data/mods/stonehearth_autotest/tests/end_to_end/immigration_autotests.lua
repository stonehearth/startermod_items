local immigration_tests = {}

function immigration_tests.get_one_citizen_encounter(autotest)
   -- Make sure we're in a good state by clearing out all bulletins
   -- and resetting the game event campaign
   stonehearth.bulletin_board:remove_all_bulletins()
   stonehearth.game_master:debug_clear_campaign('game_events')
   
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

   local cluster = autotest.env:create_entity_cluster(0, 20, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(0, 20, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(-10, 10, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(-10, 10, {size = {x=10, y=10}})

   local cluster = autotest.env:create_entity_cluster(10, 10, 10, 10, 'stonehearth:food:corn:corn_basket')
   local stockpile = autotest.env:create_stockpile(10, 10, {size = {x=10, y=10}})

   -- Wait for bulletin
   radiant.events.listen(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', function(e)
      --Click the daily update button, click the scenario
      autotest.ui:click_dom_element('#popup')
      return radiant.events.UNLISTEN
   end)

   -- Wait for score update
   autotest:sleep(1)
   local daily_report_encounter
   local function get_to_daily_report(...)
      local args = {...}
      local event_name = args[1]
      if event_name == 'elect_node' then
         local _, nodelist_name, nodes = unpack(args)
         if nodelist_name == 'game_events' then
            return 'game_events', nodes.goblin_war
         end
      elseif event_name == 'trigger_arc_edge' then
         local _, arc, edge_name, parent_node = unpack(args)
         if edge_name == 'start' then
            --Jump directly to daily_report_node
            local encounter_uri = arc._sv.encounters._sv.nodelist.daily_report
            local info =  radiant.resources.load_json(encounter_uri)
            daily_report_encounter = radiant.create_controller('stonehearth:game_master:encounter', info)
            return 'daily_report_encounter', daily_report_encounter
         end
      end
   end

   --Fire the scenario
   stonehearth.game_master:debug_campaign('game_events', function(...)
      local result = { get_to_daily_report(...) }
      if #result > 0 then
         return unpack(result)
      end
   end)

   autotest:sleep(5*1000)
   autotest.ui:click_dom_element('#acceptButton')  

   --Verify that you got a new person
   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      if stonehearth.population:get_population_size('player_1') == 2 then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   autotest:sleep(10*100000)
   autotest:fail('failed to get person')
end

return immigration_tests
