local score_tests = {}

-- Tests score fluctuations for eating
-- Should start at score 50
-- Should lose 10 for being malnourished
-- Should eat 3x, gaining 10 for eating at least once per day
-- Should lose 10 for eating 3 things of the same type
-- Should spawn a corn basket
-- Should gain 10 for eating something different 
-- Should gain 10 for eating something nutritious
-- End score should be 60
function score_tests.eat_food_tests(autotest)
   local worker = autotest.env:create_person(2, 2, {
            profession = 'worker',
            attributes = { calories = stonehearth.constants.food.MALNOURISHED }
         })
   local berry_basket = autotest.env:create_entity(-2, -2, 'stonehearth:berry_basket')

   local num_times_40 = 0
   radiant.events.listen(worker, 'stonehearth:score_changed', function(e)
         if e.new_score == 40 then
            num_times_40 = num_times_40 + 1
            if num_times_40 == 2 then
               autotest.env:create_entity(-2, -2, 'stonehearth:corn_basket')
            end
         end
         if e.new_score == 60 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(200000)
   autotest:fail('eating food score did not update correctly')
end

--Score should start at 50 and go down to 40 when there is no food at midnight. 
function score_tests.no_food_at_midnight(autotest)
   local worker = autotest.env:create_person(2, 2, {
            profession = 'worker'
         })
   stonehearth.calendar:set_time_unit_test_only({ hour = 23, minute = 58 })
   radiant.events.listen(worker, 'stonehearth:score_changed', function(e)
         if e.new_score == 40 then
            autotest:success()
            return radiant.events.UNLISTEN
         end
      end)

   autotest:sleep(20000)
   autotest:fail('midnight tick did not update correctly')
end

return score_tests