local constants = {
   TICKS_PER_SECOND = 2,

   SECONDS_IN_MINUTE = 60,
   MINUTES_IN_HOUR = 60,
   HOURS_IN_DAY = 24,
   DAYS_IN_MONTH = 30,
   MONTHS_IN_YEAR = 12,

   STARTING_MONTH = 1,
   STARTING_YEAR = 1000,

   hourNames = {
      'Aughtaur',
      'Twivaur',
      'Thrisaur',
      'Quarthaur',
      'Fifaur',
      'Sesaur',
      'Sevaur',
      'Ochaur',
      'Neinaur',
      'Tethtaur',
      'Elehaur',
      'Dovaur'
   },

   monthNames = {
      'Bittermun',
      'Deepmun',
      'Dewmun',
      'Rainmun',
      'Growmun',
      'Goldmun',
      'Feastmun',
      'Warmun',
      'Newmun',
      'Azuremun',
      'Hearthmun',
      'Northmun'
   },

   --Times of day in hours
   baseTimeOfDay = {
      midnight = 0,
      sunrise = 6,
      midday = 12,
      sunset = 18
   }

}

local data = {
   date = {
      hour = 6,
      minute = 0,
      second = 0,
      day = 0,
      month = 0,
      year = 1000
   }, --the calendar data to export
   _lastNow = 0,
   _remainderTime = 0,
   _fired_sunrise_today = false,
   _fired_noon_today = false,
   _fired_sunset_today = false,
   _fired_midnight_today = false
}

stonehearth_calendar = {}

radiant.events.register_event('radiant.events.calendar.minutely')
radiant.events.register_event('radiant.events.calendar.hourly')

radiant.events.register_event('radiant.events.calendar.sunrise')
radiant.events.register_event('radiant.events.calendar.noon')
radiant.events.register_event('radiant.events.calendar.sunset')
radiant.events.register_event('radiant.events.calendar.midnight')

function stonehearth_calendar.__init()
   radiant.events.listen('radiant.events.gameloop', stonehearth_calendar._on_event_loop)
end

function stonehearth_calendar.set_time(second, minute, hour)
   data.date.hour = hour;
   data.date.minute = minute;
   data.date.second = second;
end

-- For starters, returns base time of day.
-- TODO: change based on the season/time of year
function stonehearth_calendar.get_curr_times_of_day()
   return constants.baseTimeOfDay
end

function stonehearth_calendar.get_constants()
   return constants
end

-- recompute the game calendar based on the time
function stonehearth_calendar._on_event_loop(_, now)
   local t

   -- determine how many seconds have gone by since the last loop
   local dt = now - data._lastNow + data._remainderTime
   data._remainderTime = dt % constants.TICKS_PER_SECOND;
   local t = math.floor(dt / constants.TICKS_PER_SECOND)

   -- set the calendar data
   local sec = data.date.second + t;
   data.date.second = sec % constants.SECONDS_IN_MINUTE

   local min = data.date.minute + math.floor(sec / constants.SECONDS_IN_MINUTE)
   data.date.minute = min % constants.MINUTES_IN_HOUR

   local hour = data.date.hour + math.floor(min / constants.MINUTES_IN_HOUR)
   data.date.hour = hour % constants.HOURS_IN_DAY

   local day = data.date.day + math.floor(hour / constants.HOURS_IN_DAY)
   data.date.day = day % constants.DAYS_IN_MONTH

   local month = data.date.month + math.floor(day / constants.DAYS_IN_MONTH)
   data.date.month = month % constants.MONTHS_IN_YEAR

   local year = data.date.year + math.floor(month / constants.MONTHS_IN_YEAR)
   data.date.year = year

   if sec >= constants.SECONDS_IN_MINUTE  then
      radiant.events.broadcast_msg('radiant.events.calendar.minutely', data.date)
   end

   if min >= constants.MINUTES_IN_HOUR then
      radiant.events.broadcast_msg('radiant.events.calendar.hourly', data.date)
   end

   stonehearth_calendar.fire_time_of_day_events()

   -- the time, formatted into a string
   data.date.time = stonehearth_calendar.format_time()

   -- the date, formatting into a string
   data.date.date = stonehearth_calendar.format_date()

   data._lastNow = now
end

--[[
   If the hour is greater than the set time of day, then fire the
   relevant event.
]]
function stonehearth_calendar.fire_time_of_day_events()
   local hour = data.date.hour
   local curr_day_periods = stonehearth_calendar.get_curr_times_of_day()

   if hour >= curr_day_periods.midnight and
      hour < curr_day_periods.sunrise and
      not data._fired_midnight_today then

      radiant.events.broadcast_msg('radiant.events.calendar.midnight')
      data._fired_midnight_today = true
      
      data._fired_sunrise_today = false
      data._fired_noon_today = false
      data._fired_sunset_today = false
      return
   end

   if hour >= curr_day_periods.sunrise and
      not data._fired_sunrise_today then

      radiant.events.broadcast_msg('radiant.events.calendar.sunrise')
      data._fired_sunrise_today = true
      data._fired_midnight_today = false
      return
   end

   if hour >= curr_day_periods.midday and
      not data._fired_noon_today then

      radiant.events.broadcast_msg('radiant.events.calendar.noon')
      data._fired_noon_today = true
      return
   end

   if hour >= curr_day_periods.sunset and
      not data._fired_sunset_today then

      radiant.events.broadcast_msg('radiant.events.calendar.sunset')
      data._fired_sunset_today = true
      return
   end
end

function stonehearth_calendar.format_time()
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

function stonehearth_calendar.format_date()
   return string.format("day %d of %s, %d", data.date.day, constants.monthNames[data.date.month + 1],
      data.date.year)
end

function stonehearth_calendar.get_time_and_date()
   return data.date
end

stonehearth_calendar.__init()
return stonehearth_calendar


