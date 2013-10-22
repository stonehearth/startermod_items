local calendar = require 'services.calendar.calendar_service'

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = _radiant.sim.create_data_store()
      
      radiant.events.listen(calendar, 'stonehearth:minutely', self, self.update_clock)
      self:update_clock()
   end
   return { clock_object = clock_object }
end

function CalendarCallHandler:update_clock()
   clock_object:update(calendar:get_time_and_date())
end

return CalendarCallHandler
