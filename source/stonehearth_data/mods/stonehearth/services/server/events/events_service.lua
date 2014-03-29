EventService = class()

function EventService:initialize()

   self._sv = self.__saved_variables:get_data()
   if not self._sv.entries then
      self._entries = {}
      self._sv.entries = self._entries
   else
      self._entries = self._sv.entries
   end
end

function EventService:add_entry(text, type)
   type = type or 'info'

   local calendar = stonehearth.calendar
   local time = calendar:format_time()
   local entry = '[' .. time .. '] ' .. text
   table.insert(self._entries, entry)
   radiant.events.trigger(self, 'stonehearth:new_event', { 
      text = text,
      type = type
   })
end

return EventService
