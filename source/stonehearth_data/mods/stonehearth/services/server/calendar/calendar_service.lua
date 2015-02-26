local CalendarAlarm = require 'services.server.calendar.calendar_alarm'

local rng = _radiant.csg.get_default_rng()

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

local CalendarService = class()

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
   
   self._past_alarms = {}
   self._future_alarms = {}
   
   if not self._sv.date then
      --We're loading for the first time
      self._sv.date = {} -- the calendar data to export
      self._sv.start_time = {}
      self._sv.start_game_tick = 0

      for _, unit in ipairs(TIME_UNITS) do
         self._sv.date[unit] = self._constants.start[unit]
         self._sv.start_time[unit] = self._constants.start[unit]
      end
      self:_update_seconds_today()
   end
end

-- Returns the remaining time based on the period passed in 'm' for minute, 'h' for hour, 'd' for day, 
-- otherwise, returns period in seconds
--
function CalendarService:get_remaining_time(timer, period)   
   local seconds_remaining

   if radiant.util.is_a(timer, CalendarAlarm) then
      local alarm = timer
      local expires = alarm:get_expire_time()
      local now = self._sv.seconds_today
      
      if expires > now then
         seconds_remaining = expires - now
      else
         seconds_remaining = expires + TIME_DURATIONS.day - now
      end
   else 
      seconds_remaining = timer:get_expire_time() - self:get_elapsed_time()
   end

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

function CalendarService:format_remaining_time(timer)
   local seconds_remaining = self:get_remaining_time(timer);
   local result = ''
   
   local values = {}
   for _, v in ipairs({'day', 'hour', 'minute', 'second'}) do
      if seconds_remaining > TIME_DURATIONS[v] then
         local count = math.floor(seconds_remaining / TIME_DURATIONS[v])
         seconds_remaining = seconds_remaining - (count * TIME_DURATIONS[v])
         table.insert(values, string.format('%02d', count))
      end
   end
   return table.concat(values, ':')
end

-- returns the number of game seconds that have passed
function CalendarService:get_elapsed_time()
   return radiant.gamestate.now() / self._constants.ticks_per_second
end

-- returns the amount of stonehearth time as reported by the calendar :get_elapsed_time()
-- needs to pass for the equivalent number of realtime `seconds`.
function CalendarService:realtime_to_calendar_time(seconds)
   return (seconds * 1000) / self._constants.ticks_per_second
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
--
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

function CalendarService:parse_time(str)
   -- see if we're a fuzzy timeout
   local plus_offset = str:find('+')
   if plus_offset and plus_offset > 1 then
      local time = str:sub(1, plus_offset - 1)
      local duration = str:sub(plus_offset + 1, #str)

      local random_delay = self:parse_duration(duration)
      return self:parse_time(time) + rng:get_int(1, random_delay)
   end

   local h, m = string.match(str, '(%d+):(%d+)')
   if h == nil or m == nil then
      error(string.format("invalid time string %s", str))
   end
   h = tonumber(h)
   m = tonumber(m)
   if h < 0 or h >= self._constants.hours_per_day or m < 0 or m >= self._constants.minutes_per_hour then
      error(string.format("invalid time string %s", str))
   end
   return h * TIME_DURATIONS.hour + m * TIME_DURATIONS.minute;
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

-- alarms go off once a day
function CalendarService:set_alarm(time, fn)
   assert(type(fn) == 'function')
   
   local alarm = CalendarAlarm(time, fn)
   self:_queue_alarm(alarm)
   return alarm
end

function CalendarService:_queue_alarm(alarm)
   if alarm:get_expire_time() >= self._sv.seconds_today then
      table.insert(self._future_alarms, alarm)
      table.sort(self._future_alarms, function(l, r)
            return l:get_expire_time() < r:get_expire_time()
         end)
   else
      table.insert(self._past_alarms, alarm)
   end
end

function CalendarService:_fire_alarms(alarm)
   local now = self._sv.seconds_today

   while #self._future_alarms > 0 do
      local alarm = self._future_alarms[1]
      if alarm:get_expire_time() > now then
         break
      end
      table.remove(self._future_alarms, 1)
      if not alarm:is_dead() then
         alarm:fire()
         table.insert(self._past_alarms, alarm)
      end
   end
end

function CalendarService:_reset_all_alarms()
   for _, alarm in pairs(self._past_alarms) do
      if not alarm:is_dead() then
         alarm:reset()
         table.insert(self._future_alarms, alarm)
      end
   end
   self._past_alarms = {}
   table.sort(self._future_alarms, function(l, r)
         return l:get_expire_time() < r:get_expire_time()
      end)
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
   if self._sv.date.hour == 0 and last_hour == (self._constants.hours_per_day - 1) then
      self:_reset_all_alarms()
   end
   self:_update_seconds_today()

   -- the time, formatted into a string
   self._sv.date.time = self:format_time()

   -- the date, formatting into a string
   self._sv.date.date = self:format_date()

   self:_fire_alarms()
   self._time_tracker:set_now(self:get_elapsed_time())

   self.__saved_variables:mark_changed()

   radiant.set_performance_counter('game time', self:get_elapsed_time())
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
   self:_update_seconds_today()
   if self._sv.seconds_today == 0 then
      self:_reset_all_alarms()
   end
end

function CalendarService:_update_seconds_today()
   self._sv.seconds_today = (self._sv.date.hour * TIME_DURATIONS.hour ) + (self._sv.date.minute * TIME_DURATIONS.minute) + self._sv.date.second
   self.__saved_variables:mark_changed()
end

return CalendarService

