local calendar = require 'services.calendar.calendar_service'

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = _radiant.sim.create_data_store()
      
      local count = 0
      local update_clock = function()
         if count == 0 then
            clock_object:update(calendar:get_time_and_date())
            count = 10
         end
         count = count - 1
      end

      radiant.events.listen('radiant:events:calendar:minutely', update_clock)
      update_clock()
   end
   return { clock_object = clock_object }
end

return CalendarCallHandler
