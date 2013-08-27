local ClockDataObject = class()

function ClockDataObject:__init(data_binding)
   local calendar = radiant.mods.require('stonehearth_calendar')
   radiant.events.listen('radiant.events.calendar.minutely', function()
         self:_update(calendar, data_binding)
      end)
   self:_update(calendar, data_binding)
end

function ClockDataObject:_update(calendar, data_binding)
   data_binding:update({ date = calendar.get_time_and_date() })
end

return ClockDataObject
