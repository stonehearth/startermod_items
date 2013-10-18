local event_service = require 'services.event.event_service'

local EventCallHandler = class()

local data_object = nil

function EventCallHandler:get_events(session, request)
   if not data_object then
      radiant.log.info('calling get_events')

      local update_data_object = function()
      end
      radiant.events.listen('radiant:events:event:new_event', update_data_object)
      update_data_object()
   end
   return { events = data_object }
end

return EventCallHandler
