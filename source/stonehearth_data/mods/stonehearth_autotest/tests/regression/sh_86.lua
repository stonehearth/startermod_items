-- Dragging a stockpile around items causes workers to pickup and drop the items
-- https://bugs/browse/SH-86
--
-- 1) Make a grid of items on the ground
-- 2) Draw a stockpile around them
-- 
-- Workers will run over and attempt to put the items in the stockpile they're already in
-- 

local sh_86 = {}

function sh_86.verify_no_restocking_after_dragging_around_items(autotest) 
   autotest.env:create_person(2, 2, { job = 'worker' })
   autotest.env:create_person(4, 2, { job = 'worker' })
   autotest.env:create_person(6, 2, { job = 'worker' })

   autotest.env:create_entity_cluster(4, 4, 4, 4, 'stonehearth:resources:wood:oak_log')

   local stockpile = autotest.env:create_stockpile(-6, -6, { size = { x = 2, y = 2 } })
   local buggy_stockpile

   local counter = 0
   local buggy_count = 0

   radiant.events.listen(stockpile, 'stonehearth:stockpile:item_added', function()
         counter = counter + 1
         if counter == 1 then

            buggy_stockpile = autotest.env:create_stockpile(4, 4, { size = { x = 6, y = 6 } } )
            buggy_stockpile:set_debug_text('the buggy one')
            
            -- set a timer to ignore all the queued 'item_added' events we may get as a result of
            -- dragging out the stockpile
            radiant.set_realtime_timer("sh_86 verify_no_restocking_after_dragging_around_items", 100, function()
                  radiant.events.listen(buggy_stockpile, 'stonehearth:stockpile:item_added', function()
                        buggy_count = buggy_count + 1
                        if buggy_count > 3 then
                           -- bigger than the number of people we have.  must still be buggy!
                           autotest:fail('continued moving stuff in stockpile after created')
                        end
                     end)
               end)
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
   if not buggy_stockpile then
      autotest:fail('failed to create buggy stockpile')
      return
   end
   autotest:success()
end


function sh_86.verify_no_restocking_after_recreating_stockpile(autotest) 
   autotest.env:create_person(2, 2, { job = 'worker' })
   autotest.env:create_person(4, 2, { job = 'worker' })
   autotest.env:create_person(6, 2, { job = 'worker' })

   autotest.env:create_entity_cluster(4, 4, 4, 4, 'stonehearth:resources:wood:oak_log')

   -- create the first stockpile.  chill a bit.  then destroy it.
   local first_stockpile = autotest.env:create_stockpile(3, 3, { size = { x = 7, y = 7 } } )
   autotest:sleep(100)
   radiant.entities.destroy_entity(first_stockpile)

   -- let the noise settle, then create a 2nd stockpile.
   autotest:sleep(1000)
   local second_stockpile = autotest.env:create_stockpile(3, 3, { size = { x = 7, y = 7 } } )

   -- chill a bit to make sure the event msg queue has drained.
   autotest:sleep(100)

   -- if anyone picks up or puts down anything here, we lose.
   radiant.events.listen(second_stockpile, 'stonehearth:stockpile:item_removed', function(e)
         autotest:fail('continued moving stuff in stockpile after created (iten: %s)', e.item)
      end)
   radiant.events.listen(second_stockpile, 'stonehearth:stockpile:item_added', function(e)
         autotest:fail('continued moving stuff in stockpile after created (iten: %s)', e.item)
      end)

   autotest:sleep(2000)
   autotest:success()
end

return sh_86