
local constants = {
   TICKS_PER_SECOND = 2,

   SECONDS_IN_MINUTE = 60,
   MINUTES_IN_HOUR = 60,
   HOURS_IN_DAY = 24,
   DAYS_IN_MONTH = 30,
   MONTHS_IN_YEAR = 12,

   STARTING_MONTH = 1,
   STARTING_YEAR = 1000,

   monthNames = {
      'Bittermun',
      'Deepnum',
      'Dewmun',
      'The New Season',
      'Firstaur',
      'Secondaur',
      'Thirdaur',
      'Fourthaur',
      'Fifthaur',
      'Sixthaur',
      'Newmun',
      'Azuremun'
   }
   
}

local data = {
   calendar = {
      hour = 6,
      minute = 0,
      second = 0,
      day = 0,
      month = 0,
      year = 1000
   }, --the calendar data to export 

   _lastNow = 0,
   _remainderTime = 0,
}

local stonehearth_calendar = {}

radiant.events.register_event('radiant.events.calendar.minutely')
radiant.events.register_event('radiant.events.calendar.hourly')

function stonehearth_calendar.__init()
   radiant.events.listen('radiant.events.gameloop', stonehearth_calendar._on_event_loop)
end

function stonehearth_calendar.set_time(second, minute, hour)
   data.calendar.hour = hour;
   data.calendar.minute = minute;
   data.calendar.second = second;
end

-- recompute the game calendar based on the time
function stonehearth_calendar._on_event_loop(_, now)
   local t

   -- determine how many seconds have gone by since the last loop
   local dt = now - data._lastNow + data._remainderTime
   data._remainderTime = dt % constants.TICKS_PER_SECOND;
   local t = math.floor(dt / constants.TICKS_PER_SECOND)

   -- set the calendar data
   local sec = data.calendar.second + t;
   data.calendar.second = sec % constants.SECONDS_IN_MINUTE

   local min = data.calendar.minute + math.floor(sec / constants.SECONDS_IN_MINUTE)
   data.calendar.minute = min % constants.MINUTES_IN_HOUR

   local hour = data.calendar.hour + math.floor(min / constants.MINUTES_IN_HOUR)
   data.calendar.hour = hour % constants.HOURS_IN_DAY

   local day = data.calendar.day + math.floor(hour / constants.HOURS_IN_DAY)
   data.calendar.day = day % constants.DAYS_IN_MONTH

   local month = data.calendar.month + math.floor(day / constants.DAYS_IN_MONTH)
   data.calendar.month = month % constants.MONTHS_IN_YEAR

   local year = data.calendar.year + math.floor(month / constants.MONTHS_IN_YEAR)
   data.calendar.year = year

   if sec >= constants.SECONDS_IN_MINUTE  then
      radiant.events.broadcast_msg('radiant.events.calendar.minutely', data.calendar)
   end
   
   if min >= constants.MINUTES_IN_HOUR then
      radiant.events.broadcast_msg('radiant.events.calendar.hourly', data.calendar)
   end

   -- the time, formatted into a string
   data.calendar.time = stonehearth_calendar.format_time()

   -- the date, formatting into a string
   data.calendar.date = stonehearth_calendar.format_date()

   data._lastNow = now
end

function stonehearth_calendar.format_time()
   local suffix = "am"
   local hour = data.calendar.hour

   if data.calendar.hour == 0 then
      hour = 12
   elseif data.calendar.hour > 12 then
      hour = hour - 12
      suffix = "pm"
   end

   return string.format("%d : %02d %s", hour, data.calendar.minute, suffix)
end

function stonehearth_calendar.format_date()
   return string.format("day %d of %s, %d", data.calendar.day, constants.monthNames[data.calendar.month + 1], 
      data.calendar.year)
end

function stonehearth_calendar.get_calendar()
   return data.calendar
end

stonehearth_calendar.__init()
return stonehearth_calendar


