local stockpile_tests = {}

function stockpile_tests.restock_twice(autotest)
   autotest.env:create_person(2, 2, { job = 'worker' })
   autotest.env:create_entity(4, 4, 'stonehearth:resources:wood:oak_log')
   autotest.env:create_entity(4, 5, 'stonehearth:resources:wood:oak_log')

   local stockpile = autotest.env:create_stockpile(-2, -2)
   local counter = 0
   radiant.events.listen(stockpile, 'stonehearth:stockpile:item_added', function()
         counter = counter + 1
         if counter == 2 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(10000)
   autotest:fail('worker did not put all the logs in the stockpile')
end

return stockpile_tests
