local TICKS_PER_SECOND = 20

local SECONDS_IN_MINUTE = 60
local MINUTES_IN_HOUR = 60
local HOURS_IN_DAY = 24
local DAYS_IN_MONTH = 30
local MONTHS_IN_YEAR = 12

local STARTING_MONTH = 1
local STARTING_YEAR = 1000

local monthNames = {
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

local calendar = {}
local stonehearth_calendar = {}
local lastTicks = 0;
local previousHour = -1;
local previousMinute = -1;
local calendarStartOffset = 7 * SECONDS_IN_MINUTE * MINUTES_IN_HOUR;

radiant.events.register_event('radiant.events.calendar.minutely')
radiant.events.register_event('radiant.events.calendar.hourly')

function stonehearth_calendar.__init()
   radiant.events.listen('radiant.events.gameloop', stonehearth_calendar._on_event_loop)
end

-- recompute the game calendar based on the time
function stonehearth_calendar._on_event_loop(_, now)
   local t;

   calendar.seconds = calendarStartOffset + math.floor(now / TICKS_PER_SECOND)
   t = calendar.seconds

   -- The second
   calendar.second = math.floor(t % SECONDS_IN_MINUTE)
   t = t / SECONDS_IN_MINUTE

   -- the minute
   calendar.minute = math.floor(t % MINUTES_IN_HOUR)
   t = t / MINUTES_IN_HOUR

   if calendar.minute ~= previousMinute then
      radiant.events.broadcast_msg('radiant.events.calendar.minutely', calendar)
      previousMinute = calendar.minute
   end
   
   -- the hour
   calendar.hour = math.floor(t % HOURS_IN_DAY)
   t = t / HOURS_IN_DAY

   if calendar.hour ~= previousHour then
      radiant.events.broadcast_msg('radiant.events.calendar.hourly', calendar)
      previousHour = calendar.hour
   end

   -- the day
   calendar.day = math.floor(t % DAYS_IN_MONTH) + 1
   t = t / DAYS_IN_MONTH 

   -- the month
   calendar.month = math.floor(t % MONTHS_IN_YEAR) + STARTING_MONTH
   t = t / MONTHS_IN_YEAR

   calendar.monthName = monthNames[calendar.month]

   -- the year
   calendar.year = t + STARTING_YEAR

   -- the time, formatted into a string
   calendar.time = stonehearth_calendar.format_time()

   -- the date, formatting into a string
   calendar.date = stonehearth_calendar.format_date()

end

function stonehearth_calendar.format_time()
   local suffix = "am"
   local hour = calendar.hour

   if calendar.hour == 0 then
      hour = 12
   elseif calendar.hour > 12 then
      hour = hour - 12
      suffix = "pm"
   end

   return string.format("%d : %02d %s", hour, calendar.minute, suffix)
end

function stonehearth_calendar.format_date()
   return string.format("day %d of %s, %d", calendar.day, monthNames[calendar.month], calendar.year)
end

function stonehearth_calendar.get_calendar()
   return calendar
end

stonehearth_calendar.__init()
return stonehearth_calendar


