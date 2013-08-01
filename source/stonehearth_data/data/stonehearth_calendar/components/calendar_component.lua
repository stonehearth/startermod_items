--[[
   The calendar component manages the date and time in the game.
   Right now it just returns the date. Eventually, game pausing and
   fast-forwarding will live here.
--]]

local Calendar = class()

function Calendar:__init(entity, backing_obj)
   self._entity = entity
   self._backing_obj = backing_obj
end

--[[
   The Calendar component is initialized once and only once from
   stonehearth_calendar
--]]
function Calendar:_set_calendar(calendar)
   assert(self._calender == nil)

   self._calendar = calendar
   radiant.events.listen('radiant.events.calendar.minutely', self)
   self._backing_obj:mark_changed()
end

--[[
   When the time changes, notify
--]]
Calendar['radiant.events.calendar.minutely'] = function(self, calendar)
   self._backing_obj:mark_changed()
end

function Calendar:__tojson()
   local json = {
      date = self._calendar.get_time_and_date()
   }
   return radiant.json.encode(json)
end

return Calendar
