local EventTracker = require 'services.event.event_tracker'

local EventCallHandler = class()

function EventCallHandler:get_event_tracker(session, request)
   local event_tracker = EventTracker()
   return { tracker = event_tracker:get_data_store() }
end

return EventCallHandler