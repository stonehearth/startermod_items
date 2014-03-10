local stockpile_tests = {}

function stockpile_tests.basic_restock()
   autotest.env.create_person(2, 2, { profession = 'worker' })
   autotest.env.create_entity(4, 4, 'stonehearth:oak_log')

   local stockpile = autotest.env.create_stockpile(-2, -2)
   radiant.events.listen(stockpile, 'stonehearth:item_added', function()
         autotest.success()
      end)

   autotest.sleep(10000)
   autotest.fail('worker failed to put log in stockpile')
end

return stockpile_tests
