local stockpile_tests = {}

function stockpile_tests.basic_restock(env, runner)
   env:create_person(2, 2, { profession = 'worker' })
   env:create_entity(4, 4, 'stonehearth:oak_log')

   local stockpile = env:create_stockpile(-2, -2)
   radiant.events.listen(stockpile, 'stonehearth:item_added', function()
         runner:success()
      end)

   runner:sleep(12000)
   runner:fail('worker failed to put log in stockpile')
end

return stockpile_tests
