local data = {
   date = {
      hour = 20,
      minute = 0,
      second = 0,
      day = 0,
      month = 0,
      year = 1000
   }, --the calendar data to export
   _lastNow = 0,
   _remainderTime = 0,

   --When you change the start time change these to match
   _fired_sunrise_today = true,
   _fired_noon_today = true,
   _fired_sunset_today = false,
   _fired_midnight_today = false
}

local constants = radiant.resources.load_json('/stonehearth/services/calendar/calendar_constants.json')

CalendarService = class()

radiant.events.register_event('radiant:events:calendar:minutely')
radiant.events.register_event('radiant:events:calendar:hourly')

radiant.events.register_event('radiant:events:calendar:sunrise')
radiant.events.register_event('radiant:events:calendar:noon')
radiant.events.register_event('radiant:events:calendar:sunset')
radiant.events.register_event('radiant:events:calendar:midnight')

function CalendarService:__init()
   self._event_service = require 'services.event.event_service'
   self._constants = constants
   radiant.events.listen('radiant:events:gameloop', function (_, now)
         self:_on_event_loop(now)
      end)
end

function CalendarService:set_time(second, minute, hour)
   data.date.hour = hour;
   data.date.minute = minute;
   data.date.second = second;
end

-- For starters, returns base time of day.
-- TODO: change based on the season/time of year
function CalendarService:get_constants()
   return constants
end

-- recompute the game calendar based on the time
function CalendarService:_on_event_loop(now)
   local t

   -- determine how many seconds have gone by since the last loop
   local dt = now - data._lastNow + data._remainderTime
   data._remainderTime = dt % self._constants.ticks_per_second;
   local t = math.floor(dt / self._constants.ticks_per_second)

   -- set the calendar data
   local sec = data.date.second + t;
   data.date.second = sec % self._constants.seconds_per_minute

   local min = data.date.minute + math.floor(sec / self._constants.seconds_per_minute)
   data.date.minute = min % self._constants.minutes_per_hour

   local hour = data.date.hour + math.floor(min / self._constants.minutes_per_hour)
   data.date.hour = hour % self._constants.hours_per_day

   local day = data.date.day + math.floor(hour / self._constants.hours_per_day)
   data.date.day = day % self._constants.days_per_month

   local month = data.date.month + math.floor(day / self._constants.days_per_month)
   data.date.month = month % self._constants.months_per_year

   local year = data.date.year + math.floor(month / self._constants.months_per_year)
   data.date.year = year

   if sec >= self._constants.seconds_per_minute  then
      radiant.events.broadcast_msg('radiant:events:calendar:minutely', data.date)
   end

   if min >= self._constants.minutes_per_hour then
      radiant.events.broadcast_msg('radiant:events:calendar:hourly', data.date)
   end

   self:fire_time_of_day_events()

   -- the time, formatted into a string
   data.date.time = self:format_time()

   -- the date, formatting into a string
   data.date.date = self:format_date()

   data._lastNow = now
end

--[[
   If the hour is greater than the set time of day, then fire the
   relevant event.
]]
function CalendarService:fire_time_of_day_events()
   local hour = data.date.hour
   local curr_day_periods = self:get_constants().baseTimeOfDay

   if hour >= curr_day_periods.midnight and
      hour < curr_day_periods.sunrise and
      not data._fired_midnight_today then

      radiant.events.broadcast_msg('radiant:events:calendar:midnight')
      data._fired_midnight_today = true

      data._fired_sunrise_today = false
      data._fired_noon_today = false
      data._fired_sunset_today = false
      return
   end

   if hour >= curr_day_periods.sunrise and
      not data._fired_sunrise_today then

      radiant.events.broadcast_msg('radiant:events:calendar:sunrise')
      --xxx localise
      self._event_service:add_entry('The sun has risen on ' .. self:format_date() .. '.')
      data._fired_sunrise_today = true
      data._fired_midnight_today = false
      return
   end

   if hour >= curr_day_periods.midday and
      not data._fired_noon_today then

      radiant.events.broadcast_msg('radiant:events:calendar:noon')
      data._fired_noon_today = true
      return
   end

   if hour >= curr_day_periods.sunset and
      not data._fired_sunset_today then

      radiant.events.broadcast_msg('radiant:events:calendar:sunset')
      --xxx localize
      self._event_service:add_entry('The sun has set.')
      data._fired_sunset_today = true
      return
   end
end

function CalendarService:format_time()
   local suffix = "am"
   local hour = data.date.hour

   if data.date.hour == 0 then
      hour = 12
   elseif data.date.hour > 12 then
      hour = hour - 12
      suffix = "pm"
   end

   return string.format("%d : %02d %s", hour, data.date.minute, suffix)
end

function CalendarService:format_date()
   return string.format("day %d of %s, %d", data.date.day, self._constants.monthNames[data.date.month + 1],
      data.date.year)
end

function CalendarService:get_time_and_date()
   return data.date
end

return CalendarService()
