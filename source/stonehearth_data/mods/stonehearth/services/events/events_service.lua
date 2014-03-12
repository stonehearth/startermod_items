EventService = class()

function EventService:__init()
   self._entries = {}
   self.__savestate = radiant.create_datastore({
         entries = self._entries
      })
end

function EventService:load(savestate)
   self.__savestate = savestate
   self.__savestate:read_data(function(o)
         self._entries = o.entries
         assert(self._entries)
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
