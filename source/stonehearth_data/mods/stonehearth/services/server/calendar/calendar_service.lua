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
   self._constants = radiant.resources.load_json('/stonehearth/data/calendar/calendar_constants.json')
   
   self._time_tracker = radiant.lib.TimeTracker(self:get_elapsed_time())
   
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

   radiant.events.listen(radiant, 'stonehearth:gameloop', self, self._on_event_loop)
end

function CalendarService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv.date then
      --We're loading for the first time
      self._sv.date = {} -- the calendar data to export
      self._sv.start_time = {}
      self._sv.start_game_tick = 0

      for _, unit in ipairs(TIME_UNITS) do
         self._sv.date[unit] = self._constants.start[unit]
         self._sv.start_time[unit] = self._constants.start[unit]
      end
      
      -- When you change the start time change these to match
      self._sv._fired_sunrise_today = true
      self._sv._fired_noon_today = false
      self._sv._fired_sunset_today = false
      self._sv._fired_midnight_today = false
   end
end

--Returns the remaining time based ont he period passed in 'm' for minute, 'h' for hour, 'd' for day, 
--otherwise, returns period in seconds
--
function CalendarService:get_remaining_time(timer, period)
   local seconds_remaining = timer:get_expire_time() - self:get_elapsed_time()
   if period then
      if period == 'm' then
         return seconds_remaining / 60
      elseif period == 'h' then
         return (seconds_remaining / 60) / 60
      elseif period == 'd' then
         return ((seconds_remaining / 60) / 60) / 24
      end
   end
   return seconds_remaining 
end

-- returns the number of game seconds that have passed
function CalendarService:get_elapsed_time()
   return radiant.gamestate.now() / self._constants.ticks_per_second
end

-- returns the number of game seconds until time
function CalendarService:get_seconds_until(time)
   local duration = time - stonehearth.calendar:get_elapsed_time()
   return duration
end

-- sets a calendar timer.  
-- Either the number of seconds in gametime or a string of the form 1d1h1m1s.  zero
-- values can be omitted.  For example:
--    set_timer(120, cb)  -- 2 minute timer
--    set_timer('2m', cb) -- also a 2 minute timer
--    set_timer('1d1s', cb) -- a timer for 1 day and 1 second.
function CalendarService:set_timer(duration, fn)
   return self:_create_timer(duration, fn, false)
end

function CalendarService:set_interval(duration, fn)
   return self:_create_timer(duration, fn, true)
end

-- parses a duration string into game seconds
function CalendarService:parse_duration(str)
   local get_value = function(str, suffix, multiplier)
      local quantity = string.match(str, '(%d+)' .. suffix) or 0
      return quantity * multiplier
   end

   local seconds = get_value(str, 'Y', TIME_DURATIONS.year) +
                   get_value(str, 'M', TIME_DURATIONS.month) +
                   get_value(str, 'd', TIME_DURATIONS.day) +
                   get_value(str, 'h', TIME_DURATIONS.hour) +
                   get_value(str, 'm', TIME_DURATIONS.minute) +
                   get_value(str, 's', TIME_DURATIONS.second)
   return seconds
end

-- currently zero duration timers will fire on the following game loop
function CalendarService:_create_timer(duration, fn, repeating)
   assert(type(fn) == 'function')
   
   local timeout_s

   if type(duration) == 'string' then
      timeout_s = self:parse_duration(duration)
   else
      timeout_s = duration
   end
   assert(timeout_s >= 0, string.format('invalid duration passed to calendar set timer, "%s"', tostring(duration)))

   return self._time_tracker:set_timer(timeout_s, fn, repeating)
end

function CalendarService:get_constants()
   return self._constants
end

-- recompute the game calendar based on the time
function CalendarService:_on_event_loop(e)
   local last_hour = self._sv.date.hour 
   local elapased = e.now - self._sv.start_game_tick

   local remaining = math.floor(elapased / self._constants.ticks_per_second)

   -- compute the time based on how much time has passed and the time offset.
   local date = self._sv.date
   for _, unit in ipairs(TIME_UNITS) do
      remaining = self._sv.start_time[unit] + remaining
      date[unit] = math.floor(remaining % TIME_INTERVALS[unit])
      remaining = remaining - date[unit]
      if remaining == 0  then
         break
      end
      remaining = remaining / TIME_INTERVALS[unit]
   end

   if last_hour ~= nil and last_hour ~= self._sv.date.hour then
      radiant.events.trigger_async(self, 'stonehearth:hourly', { now = self._sv.date })
   end

   self:fire_time_of_day_events()

   -- the time, formatted into a string
   self._sv.date.time = self:format_time()

   -- the date, formatting into a string
   self._sv.date.date = self:format_date()

   self._time_tracker:set_now(self:get_elapsed_time())
   self.__saved_variables:mark_changed()

   radiant.set_performance_counter('game time', self:get_elapsed_time())
end

--[[
   If the hour is greater than the set time of day, then fire the
   relevant event.
]]
function CalendarService:fire_time_of_day_events()
   local hour = self._sv.date.hour
   local curr_day_periods = self._constants.event_times

   if hour >= curr_day_periods.midnight and
      hour < curr_day_periods.sunrise and
      not self._sv._fired_midnight_today then

      radiant.events.trigger_async(self, 'stonehearth:midnight')
      self._sv._fired_midnight_today = true

      self._sv._fired_sunrise_today = false
      self._sv._fired_noon_today = false
      self._sv._fired_sunset_today = false
      return
   end

   if hour >= curr_day_periods.sunrise and
      not self._sv._fired_sunrise_today then

      radiant.events.trigger_async(self, 'stonehearth:sunrise')
      --xxx localise
      stonehearth.events:add_entry('The sun has risen on ' .. self:format_date() .. '.')
      self._sv._fired_sunrise_today = true
      self._sv._fired_midnight_today = false
      return
   end

   if hour >= curr_day_periods.midday and
      not self._sv._fired_noon_today then

      radiant.events.trigger_async(self, 'stonehearth:noon')
      self._sv._fired_noon_today = true
      return
   end

   if hour >= curr_day_periods.sunset and
      not self._sv._fired_sunset_today then

      radiant.events.trigger_async(self, 'stonehearth:sunset')
      --xxx localize
      stonehearth.events:add_entry('The sun has set.')
      self._sv._fired_sunset_today = true
      return
   end
end

function CalendarService:format_time()
   local suffix = "am"
   local hour = self._sv.date.hour

   if self._sv.date.hour == 0 then
      hour = 12
   elseif self._sv.date.hour > 12 then
      hour = hour - 12
      suffix = "pm"
   end

   return string.format("%d : %02d %s", hour, self._sv.date.minute, suffix)
end

function CalendarService:format_date()
   -- all time units are stored in base 0, but days and months are displayed in base 1
   return string.format("day %d of %s, %d", self._sv.date.day + 1, self._constants.month_names[self._sv.date.month + 1],
      self._sv.date.year)
end

function CalendarService:get_time_and_date()
   return self._sv.date
end

---Use in tests to change the time of day. 
-- @param unit - pick hour, min, day, etc
-- @param value - a numeric value appropriate to the unit
function CalendarService:set_time_unit_test_only(override)
   -- make the start time equal to the current time, overriding stuff
   -- that the user wants to manaully set
   for name, value in pairs(self._sv.start_time) do
      local new_value = override[name] or self._sv.date[name]
      self._sv.date[name] = new_value
      self._sv.start_time[name] = new_value
   end
   self._sv.start_game_tick = radiant.gamestate.now()
end


return CalendarService

