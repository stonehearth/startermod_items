local event_service = require 'services.event.event_service'

local EventCallHandler = class()

local data_object = nil

function EventCallHandler:get_events(session, request)
   if not data_object then
      radiant.log.info('calling get_events')
      data_object = _radiant.sim.create_data_store()
      data_object:update(event_service:get_entries())

      radiant.events.listen(event_service, 'stonehearth:new_event', self, self.update_data_object)
      self:update_data_object()
   end
   return { events = data_object }
end

function EventCallHandler:update_data_object( e )
   data_object:mark_changed()
end

return EventCallHandler
