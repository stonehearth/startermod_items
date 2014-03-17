local calendar = stonehearth.calendar

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = radiant.create_datastore()
      stonehearth.calendar:set_interval('1m', function(e)
            clock_object:set_data(calendar:get_time_and_date())
         end)
      clock_object:set_data(calendar:get_time_and_date())
   end
   return { clock_object = clock_object }
end

return CalendarCallHandler
