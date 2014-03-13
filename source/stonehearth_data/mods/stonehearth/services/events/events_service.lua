EventService = class()

function EventService:initialize()
   self._entries = {}
   self.__saved_variables = radiant.create_datastore({
         entries = self._entries
      })
end

function EventService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self.__saved_variables:read_data(function(o)
         self._entries = o.entries or {}
      end)
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
