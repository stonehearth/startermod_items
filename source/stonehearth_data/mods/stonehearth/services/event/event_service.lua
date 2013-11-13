EventService = class()

function EventService:__init()
   self._entries = {}
end

function EventService:add_entry(text, type)
   type = type or 'info'

   local calendar = require 'services.calendar.calendar_service'
   local time = calendar:format_time()
   local entry = '[' .. time .. '] ' .. text
   table.insert(self._entries, entry)
   radiant.events.trigger(self, 'stonehearth:new_event', { 
      text = text,
      type = type
   })
end

function EventService:get_entries()
   return self._entries
end

return EventService()