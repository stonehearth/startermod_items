local alarms_autotest = {}

local function verify_time(autotest, hour, minute)
   local now = stonehearth.calendar:get_time_and_date()
   if now.hour ~= hour or now.minute ~= minute then
      autotest:fail('alarm fired at unexpected time %d:%02d (should be %d:%02d)', now.hour, now.minute, hour, minute)
   end
end

local function set_alarm(h, m, cb)
   local when = string.format('%d:%02d', h, m)
   return stonehearth.calendar:set_alarm(when, cb)
end

function alarms_autotest.does_future_alarm_fire(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = 12, minute = 0 })

   local timer, alarm
   alarm = stonehearth.calendar:set_alarm('12:02', function()
         alarm:destroy()
         timer:destroy()
         verify_time(autotest, 12, 2)
         autotest:success()
      end)
      
   timer = stonehearth.calendar:set_timer('5m', function()
         autotest:fail('alarm failed to fire')
      end)
      
   autotest:sleep(5 * 1000)
   autotest:fail('test failed to complete in a reasonable amount of time')
end

function alarms_autotest.does_past_alarm_not_fire(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = 12, minute = 0 })

   local timer, alarm
   alarm = stonehearth.calendar:set_alarm('11:59', function()
         autotest:fail('alarm in past fired')
      end)
      
   timer = stonehearth.calendar:set_timer('2m', function()
         alarm:destroy()
         timer:destroy()
         autotest:success()
      end)
      
   autotest:sleep(5 * 1000)
   autotest:fail('test failed to complete in a reasonable amount of time')
end

function alarms_autotest.does_alarm_fire_next_day(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = 23, minute = 55 })

   local day = 0
   local timer, alarm
   alarm = stonehearth.calendar:set_alarm('0:02', function()
         if day == 0 then
            autotest:fail('alarm a day too early')
         end
         if day == 1 then
            alarm:destroy()
            timer:destroy()
            verify_time(autotest, 0, 2)
            autotest:success()
         end         
      end)
      
   timer = stonehearth.calendar:set_timer('5m', function()
         day = day + 1
         timer:destroy()
      end)
   
   autotest:sleep(5 * 1000)
   autotest:fail('test failed to complete in a reasonable amount of time')
end


function alarms_autotest.fuzzy_alarm(autotest)
   stonehearth.calendar:set_time_unit_test_only({ hour = 12, minute = 0 })

   local timer, alarm
   alarm = stonehearth.calendar:set_alarm('12:01+2m', function()
         alarm:destroy()
         timer:destroy()
         local now = stonehearth.calendar:get_time_and_date()
         if now.hour ~= 12 or now.minute < 1 or now.minute > 3 then
            autotest:fail('alarm fired at unexpected time %d:%02d (should be between 12:01 and 12:03)', now.hour, now.minute)
         end
         autotest:success()
      end)
      
   timer = stonehearth.calendar:set_timer('5m', function()
         autotest:fail('alarm failed to fire')
      end)
      
   autotest:sleep(5 * 1000)
   autotest:fail('test failed to complete in a reasonable amount of time')
end

return alarms_autotest