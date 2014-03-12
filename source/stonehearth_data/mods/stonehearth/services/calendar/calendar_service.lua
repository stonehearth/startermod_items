
CalendarService = class()

function CalendarService:__init()
   self._timers = {}
   self._constants = radiant.resources.load_json('/stonehearth/services/calendar/calendar_constants.json')
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._on_event_loop)
end

function CalendarService:initialize()
   self._data = {
      --the calendar data to export
      date = {
         second = 0,
         hour = self._constants.start.hour,
         minute = self._constants.start.minute,
         day = self._constants.start.day,
         month = self._constants.start.month,
         year = self._constants.start.year
      },

      _lastNow = 0,
      _remainderTime = 0,

      -- When you change the start time change these to match
      _fired_sunrise_today = true,
      _fired_noon_today = true,
      _fired_sunset_today = false,
      _fired_midnight_today = false
   }
   self.__saved_variables = radiant.create_datastore(self._data)
end

function CalendarService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self._data = saved_variables:get_data()
end

function CalendarService:set_time(hour, minute, second)
   self._data.date.hour = hour;
   self._data.date.minute = minute;
   self._data.date.second = second;
end

function CalendarService:set_timer(hours, minutes, seconds, fn)
   local timer_length = (hours * self._constants.minutes_per_hour * self._constants.seconds_per_minute ) +
                            (minutes * self._constants.seconds_per_minute) + seconds


   local timer = {
      length = timer_length,
      fn = fn
   }
   table.insert(self._timers, timer)
   return timer
end

function CalendarService:remove_timer(t)
   for i, timer in ipairs(self._timers) do
      if timer == t then
         table.remove(self._timers, i)
         return
      end
   end
end

function CalendarService:get_constants()
   return self._constants
end

-- recompute the game calendar based on the time
function CalendarService:_on_event_loop(e)
   local now = e.now

   -- determine how many seconds have gone by since the last loop
   local dt = now - self._data._lastNow + self._data._remainderTime
   self._data._remainderTime = dt % self._constants.ticks_per_second;
   local t = math.floor(dt / self._constants.ticks_per_second)

   -- set the calendar data
   local sec = self._data.date.second + t;
   self._data.date.second = sec % self._constants.seconds_per_minute

   local min = self._data.date.minute + math.floor(sec / self._constants.seconds_per_minute)
   self._data.date.minute = min % self._constants.minutes_per_hour

   local hour = self._data.date.hour + math.floor(min / self._constants.minutes_per_hour)
   self._data.date.hour = hour % self._constants.hours_per_day

   local day = self._data.date.day + math.floor(hour / self._constants.hours_per_day)
   self._data.date.day = day % self._constants.days_per_month

   local month = self._data.date.month + math.floor(day / self._constants.days_per_month)
   self._data.date.month = month % self._constants.months_per_year

   local year = self._data.date.year + math.floor(month / self._constants.months_per_year)
   self._data.date.year = year

   if sec >= self._constants.seconds_per_minute  then
      radiant.events.trigger(self, 'stonehearth:minutely', { now = self._data.date})
   end

   if min >= self._constants.minutes_per_hour then
      radiant.events.trigger(self, 'stonehearth:hourly', {now = self._data.date})
   end

   self:fire_time_of_day_events()

   -- the time, formatted into a string
   self._data.date.time = self:format_time()

   -- the date, formatting into a string
   self._data.date.date = self:format_date()

   self:update_timers(t)

   self._data._lastNow = now

   self.__saved_variables:mark_changed()
end

function CalendarService:update_timers(dt)
   for i, timer in ipairs(self._timers) do
      timer.length = timer.length - dt
      if timer.length <= 0 then
         timer.fn()
         table.remove(self._timers, i)
      end
   end
end
--[[
   If the hour is greater than the set time of day, then fire the
   relevant event.
]]
function CalendarService:fire_time_of_day_events()
   local hour = self._data.date.hour
   local curr_day_periods = self._constants.event_times

   if hour >= curr_day_periods.midnight and
      hour < curr_day_periods.sunrise and
      not self._data._fired_midnight_today then

      radiant.events.trigger(self, 'stonehearth:midnight')
      self._data._fired_midnight_today = true

      self._data._fired_sunrise_today = false
      self._data._fired_noon_today = false
      self._data._fired_sunset_today = false
      return
   end

   if hour >= curr_day_periods.sunrise and
      not self._data._fired_sunrise_today then

      radiant.events.trigger(self, 'stonehearth:sunrise')
      --xxx localise
      stonehearth.events:add_entry('The sun has risen on ' .. self:format_date() .. '.')
      self._data._fired_sunrise_today = true
      self._data._fired_midnight_today = false
      return
   end

   if hour >= curr_day_periods.midday and
      not self._data._fired_noon_today then

      radiant.events.trigger(self, 'stonehearth:noon')
      self._data._fired_noon_today = true
      return
   end

   if hour >= curr_day_periods.sunset and
      not self._data._fired_sunset_today then

      radiant.events.trigger(self, 'stonehearth:sunset')
      --xxx localize
      stonehearth.events:add_entry('The sun has set.')
      self._data._fired_sunset_today = true
      return
   end
end

function CalendarService:format_time()
   local suffix = "am"
   local hour = self._data.date.hour

   if self._data.date.hour == 0 then
      hour = 12
   elseif self._data.date.hour > 12 then
      hour = hour - 12
      suffix = "pm"
   end

   return string.format("%d : %02d %s", hour, self._data.date.minute, suffix)
end

function CalendarService:format_date()
   return string.format("day %d of %s, %d", self._data.date.day, self._constants.month_names[self._data.date.month + 1],
      self._data.date.year)
end

function CalendarService:get_time_and_date()
   return self._data.date
end

return CalendarService

