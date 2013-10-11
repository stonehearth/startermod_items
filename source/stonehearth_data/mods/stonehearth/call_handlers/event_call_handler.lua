local event_service = require 'services.event.event_service'

local EventCallHandler = class()

local data_object = nil

function EventCallHandler:get_events(session, request)
   if not data_object then
      radiant.log.info('calling get_events')
      data_object = _radiant.sim.create_data_store()
      data_object:update(event_service:get_entries())
      
      local update_data_object = function()
         data_object:mark_changed()
      end

      radiant.events.listen('radiant:events:event:new_event', update_data_object)
      
      update_data_object()
   end
   return { events = data_object }
end

return EventCallHandler
