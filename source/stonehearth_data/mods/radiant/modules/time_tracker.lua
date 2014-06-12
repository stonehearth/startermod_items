TimeTracker = class()
local Timer = class()

function Timer:__init(start_time, duration, fn, repeating)
   self._active = true
   self._start_time = start_time
   self._duration = duration
   self._expire_time = start_time + duration
   self._repeating = repeating
   self._fn = fn
end

function Timer:destroy()
   self._active = false
end

function Timer:is_active()
   return self._active
end

--This way you can get the new expire time during the callback, if so desired
function Timer:fire()
   if self._repeating then
      self._expire_time = self._expire_time + self._duration
   else
      self._active = false
   end
   self._fn()
end

function Timer:get_expire_time()
   return self._expire_time
end

function Timer:get_remaining_time()
   return self:get_expire_time() - stonehearth.calendar:get_elapsed_game_time()
end


-- TimeTracker

function TimeTracker:__init()
   self._timers = {}
   self._now = 0
end

function TimeTracker:set_elapsed_time(now)
   self._now = now

   self._next_timers = {}
   local elapsed = self:get_elapsed_time()
   for i, timer in ipairs(self._timers) do
      if timer:is_active() then
         if timer:get_expire_time() <= elapsed then
            timer:fire()
         end
         if timer:is_active() then
            table.insert(self._next_timers, timer)
         end
      end
   end
   self._timers = self._next_timers
   self._next_timers = nil
end

function TimeTracker:get_elapsed_time()
   return self._now
end

function TimeTracker:set_interval(duration, fn)
   return self:set_timer(duration, fn, true)
end

function TimeTracker:set_timer(duration, fn, repeating)
   local timer = Timer(self:get_elapsed_time(), duration, fn, repeating)

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

return TimeTracker
