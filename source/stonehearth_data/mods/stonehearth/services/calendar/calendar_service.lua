CalendarTimer = require 'services.calendar.calendar_timer'
CalendarService = class()


local TIME_UNITS = {
   'second',
   'minute',
   'hour',
   'day',
   'month',
   'year',
}

local TIME_INTERVALS = {}
local TIME_DURATIONS = {}

function CalendarService:__init()
   self._timers = {}
   self._constants = radiant.resources.load_json('/stonehearth/services/calendar/calendar_constants.json')
   
   
   -- build the TIME_UNIT_DURATION_S table containing how many seconds each unit of time is   
   TIME_INTERVALS.second = self._constants.seconds_per_minute
   TIME_INTERVALS.minute = self._constants.minutes_per_hour
   TIME_INTERVALS.hour = self._constants.hours_per_day
   TIME_INTERVALS.day = self._constants.days_per_month
   TIME_INTERVALS.month = self._constants.months_per_year

   TIME_DURATIONS.second = 1
   TIME_DURATIONS.minute = TIME_DURATIONS.second * self._constants.seconds_per_minute
   TIME_DURATIONS.hour = TIME_DURATIONS.minute * self._constants.minutes_per_hour
   TIME_DURATIONS.day = TIME_DURATIONS.hour * self._constants.hours_per_day
   TIME_DURATIONS.month = TIME_DURATIONS.day * self._constants.days_per_month
   TIME_DURATIONS.year = TIME_DURATIONS.month * self._constants.months_per_year

   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._on_event_loop)
end

function CalendarService:initialize()
   self._data = {
      --the calendar data to export
      date = {},
      
      -- When you change the start time change these to match
      _fired_sunrise_today = true,
      _fired_noon_today = true,
      _fired_sunset_today = false,
      _fired_midnight_today = false
   }
   for _, unit in ipairs(TIME_UNITS) do
      self._data.date[unit] = self._constants.start[unit]
   end
   self._current_game_seconds = 0
   self.__saved_variables = radiant.create_datastore(self._data)
end

function CalendarService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self._data = saved_variables:get_data()
end

function CalendarService:get_elapsed_game_time()
   return self._current_game_seconds
end

-- sets a calendar timer.  
-- Either the number of seconds in gametime or a string of the form 1d1h1m1s.  zero
-- values can be omitted.  For example:
--    set_timer(120, cb)  -- 2 minute timer
--    set_timer('2m', cb) -- also a 2 minute timer
--    set_timer('1d1s', cb) -- a timer for 1 day and 1 second.

function CalendarService:set_timer(duration, fn)
   self:_create_timer(duration, fn, false)
end

function CalendarService:set_interval(duration, fn)
   self:_create_timer(duration, fn, true)
end

function CalendarService:_create_timer(duration, fn, repeating)
   assert(type(fn) == 'function')
   
   local timeout_s = 0

   if type(duration) == 'string' then
      local function match(f, d)
         return d * (string.match(duration, '(%d+)' .. f) or 0)
      end
      timeout_s = match('Y', TIMER_DURATIONS.year) +
                  match('M', TIMER_DURATIONS.month) +
                  match('d', TIMER_DURATIONS.day) +
                  match('h', TIMER_DURATIONS.hour)  +
                  match('m', TIMER_DURATIONS.minute) +
                  match('s', TIMER_DURATIONS.second)
   else
      timeout_s = duration
   end
   assert(timeout_s > 0, 'invalid duration passed to calendar set timer, "%s"', tostring(duration))

   local timer = CalendarTimer(self._current_game_seconds + timeout_s, fn, repeating)

   -- if we're currently firing timers, the _next_timers variable will contain the timers
   -- we'll check next gameloop. stick the timer in there instead of the timers array.  this
   -- prevents us from modifying the _timers array during iteration, which could make us
   -- accidently skip timers.
   if self._next_timers then
      table.insert(self._next_timers, timer)
   else
      table.insert(self._timers, timer)
   end
   return timer
end

function CalendarService:get_constants()
   return self._constants
end

-- recompute the game calendar based on the time
function CalendarService:_on_event_loop(e)
   local now = e.now

   local last_hour = self._data.date.hour 

   self._current_game_seconds = math.floor(e.now / self._constants.ticks_per_second)

   -- compute the time based on how much time has passed and the time offset.
   local date = self._data.date
   local time_reminaing = self._current_game_seconds
   for _, unit in ipairs(TIME_UNITS) do
      time_reminaing = self._constants.start[unit] + time_reminaing
      date[unit] = math.floor(time_reminaing % TIME_INTERVALS[unit])
      time_reminaing = time_reminaing - date[unit]
      if time_reminaing == 0  then
         break
      end
      time_reminaing = time_reminaing / TIME_INTERVALS[unit]
   end

   if last_hour ~= nil and last_hour ~= self._data.date.hour then
      radiant.events.trigger(self, 'stonehearth:hourly', { now = self._data.date })
   end

   self:fire_time_of_day_events()

   -- the time, formatted into a string
   self._data.date.time = self:format_time()

   -- the date, formatting into a string
   self._data.date.date = self:format_date()

   self:_update_timers()
   self.__saved_variables:mark_changed()
end

function CalendarService:_update_timers()
   self._next_timers = {}
   for i, timer in ipairs(self._timers) do
      if timer.active then
         if timer.expire_time <= self._current_game_seconds then
            timer.fn()
            timer.active = timer.repeating
         end
         if timer.active then
            table.insert(self._next_timers, timer)
         end
      end
   end
   self._timers = self._next_timers
   self._next_timers = nil
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

