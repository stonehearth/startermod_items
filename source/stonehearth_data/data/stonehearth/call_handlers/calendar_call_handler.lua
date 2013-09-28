local calendar = require 'services.calendar.calendar_service'

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = _radiant.sim.create_data_store()
      
      local update_clock = function()
         clock_object:update(calendar:get_time_and_date())
      end

      radiant.events.listen('radiant.events.calendar.minutely', update_clock)
      update_clock()
   end
   return { clock_object = clock_object }
end

return CalendarCallHandler
