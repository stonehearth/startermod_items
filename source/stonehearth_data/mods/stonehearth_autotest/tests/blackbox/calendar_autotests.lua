local calendar_tests = {}

function calendar_tests.check_constants(autotest)
   local expected = {
      ticks_per_second = 9,
      seconds_per_minute = 60,
      minutes_per_hour = 60,
      hours_per_day = 24,
      days_per_month = 30,
      months_per_year = 12,

      start = {
         minute = 0,
         hour = 8,
         day = 1,
         month = 1,
         year = 1000,
      },

      event_times = {
         midnight = 0,
         sunrise = 6,
         midday = 14,
         sunset_start = 20,
         sunset = 22,
      },
   }
   local success, err = autotest.util:check_table(expected, stonehearth.calendar:get_constants())
   if not success then
      autotest:fail('calendar constants wrong: %s', err)
   end
   autotest:success()
end

return calendar_tests
