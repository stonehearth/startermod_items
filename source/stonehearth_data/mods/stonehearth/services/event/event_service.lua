EventService = class()

radiant.events.register_event('radiant:events:event:new_event')

function EventService:__init()
   self._entries = {}
   self._calendar_service = require 'services.calendar.calendar_service'

   self._debug_spew = false;

   radiant.events.listen('radiant:events:calendar:sunrise', function (_, now)
         -- the sun has risen
   end)

   radiant.events.listen('radiant:events:calendar:hourly', function (_, now)
      if self._debug_spew then
         self:add_entry('the hour is now ' .. tostring(now.hour))
      end
   end)

end

function EventService:add_entry(text)
   local calendar = require 'services.calendar.calendar_service'
   local time = calendar:format_time()
   local entry = '[' .. time .. '] ' .. text
   table.insert(self._entries, entry)
   radiant.events.broadcast_msg('radiant:events:event:new_event', text)
end

function EventService:get_entries()
   return self._entries
end

return EventService()