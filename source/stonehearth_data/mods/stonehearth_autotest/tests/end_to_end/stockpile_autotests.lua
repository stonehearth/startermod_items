local Point3f = _radiant.csg.Point3f

local stockpile_tests = {}

-- make sure we can put 2 logs into the stockpile
function stockpile_tests.restock_twice(autotest)
   autotest.env:create_person(2, 2, { profession = 'worker' })
   autotest.env:create_entity(4, 4, 'stonehearth:oak_log')
   autotest.env:create_entity(4, 5, 'stonehearth:oak_log')

   local stockpile = autotest.env:create_stockpile(-2, -2)
   local counter = 0
   radiant.events.listen(stockpile, 'stonehearth:item_added', function()
         counter = counter + 1
         if counter == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
   autotest:fail('worker did not put all the logs in the stockpile')
end


-- Sets the 'none' bit of the stockpile while the worker is getting ready to carry a log there.
-- This reproduces a nasty AI bug that we don't want showing up again.
function stockpile_tests.abort_stockpile_creation(autotest)
   local carry_change_count = 0
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local wood = autotest.env:create_entity_cluster(-2, -2, 3, 3, 'stonehearth:oak_log')
   local stockpile = autotest.env:create_stockpile(4, 8)

   radiant.events.listen(worker, 'stonehearth:carry_block:carrying_changed', function (e)
      carry_change_count = carry_change_count + 1
      if carry_change_count == 1 then

         autotest.ui:select_entity(stockpile)
         autotest.ui:click_dom_element('#stockpileWindow #none')
         autotest.ui:click_dom_element('.stonehearthMenu .close')

      elseif carry_change_count == 2 then
         if autotest:script_did_error() then
            autotest:fail('Encountered error during execution.')
         else
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end
      end)

   autotest:sleep(10000)
   autotest:fail('Entity didn\'t drop its carry block.')
end

-- make sure we don't pickup items already stocked
function stockpile_tests.dont_pickup_stocked_items(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local stockpile = autotest.env:create_stockpile(3, 3, { size = { x = 3, y = 3 }})

   radiant.events.listen(worker, 'stonehearth:carry_block:carrying_changed', function()
         autotest:fail('worker should not have picked up log already in stockpile')
      end)

   -- the potentially test killing log!
   autotest.env:create_entity(4, 4, 'stonehearth:oak_log')


   autotest:sleep(1000)
   autotest:success()
end

-- make sure we don't pickup items already stocked
function stockpile_tests.restock_items_when_falling_out_of_filter(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local stockpile_a = autotest.env:create_stockpile( 3, 3, { size = { x = 3, y = 3 }})
   local stockpile_b = autotest.env:create_stockpile(-3, 3, { size = { x = 3, y = 3 }})

   radiant.events.listen(stockpile_b, 'stonehearth:item_added', function()
         autotest:success()
         return radiant.events.UNLISTEN
      end)

   -- this log's in stockpile_a...
   autotest.env:create_entity(4, 4, 'stonehearth:oak_log')

   -- wait a little bit and change a's filter to include nothing.  this should
   -- make the worker pick it up and move it to b
   autotest:sleep(100)
   stockpile_a:get_component('stonehearth:stockpile'):set_filter({})

   autotest:sleep(5000)
   autotest:fail('worker failed to move item when stockpile filter changes')
end

-- make sure we can find newly created items to stock, even after we've started
-- looking for them.  this is mainly a test of the bfs pathfinder.
function stockpile_tests.restock_newly_created_items(autotest)
   local worker = autotest.env:create_person(2, 2, { profession = 'worker' })
   local stockpile = autotest.env:create_stockpile(-2, -2)

   -- wait a bit to give the pathfinder room to not find anything, then add
   -- a log to the world
   autotest:sleep(500)

   -- now make a log.  we're successful if the restock task decides to pick it up
   radiant.events.listen(worker, 'stonehearth:carry_block:carrying_changed', function (e)
         autotest:success()
         return radiant.events.UNLISTEN
      end)
   autotest.env:create_entity(4, 4, 'stonehearth:oak_log')

   autotest:sleep(1000)
   autotest:fail('worker did not find newly created log')
end

return stockpile_tests
