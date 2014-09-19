local score_tests = {}

local function place_town_banner(autotest)
   local banner = autotest.env:create_entity(0,0, 'stonehearth:camp_standard', { force_iconic = false })
   local town = stonehearth.town:get_town(autotest.env.get_player_id())
   town:set_banner(banner)
end

-- Spawn an immigration failure scenario, confirm trade
function score_tests.immigration_failure(autotest)
   place_town_banner(autotest)
   local worker = autotest.env:create_person(2, 2, {
         job = 'worker'
      })

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      local uri = e.entity:get_uri()
      if uri == 'stonehearth:berry_basket' or 
         uri == 'stonehearth:turnip_basket' or 
         uri == 'stonehearth:corn_basket' or 
         uri == 'stonehearth:rabbit_jerky' or 
         uri == 'stonehearth:farmer:hoe_talisman'
         then
         autotest:success()
         return radiant.events.UNLISTEN
      end
   end)

   radiant.events.listen(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', function(e)
      autotest.ui:click_dom_element('#popup')
      autotest.ui:click_dom_element('#acceptButton')    
      return radiant.events.UNLISTEN  
   end)

   --Try to spawn the immigration_failure scenario
   stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:immigration_failure')

   autotest.util:fail_if_expired(60 * 1000, 'failed to get stuff from traveller')
end

-- Have a caravan come by and offer for the berry baskets in the stockpile
function score_tests.simple_carvan(autotest)
   place_town_banner(autotest)
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })

   local stockpile = autotest.env:create_stockpile(-2, -2)
   local log = autotest.env:create_entity(3, 3, 'stonehearth:oak_log')
   local has_taken_object = false
   local has_given_object = false

   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function (e)
      if not log:is_valid() then
         has_taken_object = true
         if has_given_object and has_taken_object then
            autotest:success()
         end
         return radiant.events.UNLISTEN
      end
   end)

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      has_given_object = true
      if has_given_object and has_taken_object then
         autotest:success()
      end
      return radiant.events.UNLISTEN
   end)

   --Wait until the person has put the thing in the stockpile
   radiant.events.listen(stockpile, 'stonehearth:item_added', function(e)
      stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:simple_caravan')
      return radiant.events.UNLISTEN
   end)

   radiant.events.listen(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', function(e)
      autotest.ui:click_dom_element('#popup')
      autotest.ui:click_dom_element('#acceptButton')  
      return radiant.events.UNLISTEN    
   end)

   autotest.util:fail_if_expired(60 * 1000, 'failed to get caravan')
end


-- Spawn an immigrant when the situation is correct
function score_tests.immigrant(autotest)
   place_town_banner(autotest)
   local worker = autotest.env:create_person(2, 2, { job = 'worker' })

   radiant.events.listen(radiant, 'radiant:entity:post_create', function (e)
      autotest:success()
      return radiant.events.UNLISTEN
   end)

   radiant.events.listen(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', function(e)
      --Again, a little fragile
      autotest.ui:click_dom_element('#popup')
      autotest.ui:click_dom_element('#acceptButton')   
      return radiant.events.UNLISTEN   
   end)

   --The force spawn ignores "can spawn"
   stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:immigration')

   autotest.util:fail_if_expired(60 * 1000, 'failed to get immigrant')
end


return score_tests
