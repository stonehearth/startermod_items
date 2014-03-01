local calendar = stonehearth.calendar

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = _radiant.sim.create_datastore(self)
      radiant.events.listen(calendar, 'stonehearth:minutely', function(e)
            clock_object:update(calendar:get_time_and_date())
         end)
      clock_object:update(calendar:get_time_and_date())
   end
   return { clock_object = clock_object }
end

return CalendarCallHandler
