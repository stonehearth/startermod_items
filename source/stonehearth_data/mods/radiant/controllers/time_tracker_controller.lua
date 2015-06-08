local TimeTracker = class()

function TimeTracker:initialize()
   self._sv._timers = {}
   self._sv._now = 0
end

function TimeTracker:set_now(now)
   self._sv._now = now

   -- If we're saving, then the whole Lua VM is brought down, quite likely with _next_timers 
   -- carrying a payload.  If so, then don't clear the timers that are there.
   if not self._sv._next_timers then
      self._sv._next_timers = {}
   end

   for _, timer in pairs(self._sv._timers) do
      if timer:is_active() then
         if timer:get_expire_time() <= now then
            -- fire the timer in a pcall...
            xpcall(function() timer:fire(now) end, radiant.report_traceback)
         end
         if timer:is_active() then
            table.insert(self._sv._next_timers, timer)
         end
      end
   end
   self._sv._timers = self._sv._next_timers
   self._sv._next_timers = nil

   self.__saved_variables:mark_changed()
end

function TimeTracker:get_now()
   return self._sv._now
end

function TimeTracker:increment_now(delta)
   local new_time = self._sv._now + delta
   self:set_now(new_time)
end

function TimeTracker:set_interval(duration, fn)
   return self:set_timer(duration, fn, true)
end

function TimeTracker:set_timer(duration, fn, repeating)   
   local timer = radiant.create_controller('radiant:controllers:timer', self:get_now(), duration, fn, repeating)

   -- if we're currently firing timers, the _next_timers variable will contain the timers
   -- we'll check next gameloop. stick the timer in there instead of the timers array.  this
   -- prevents us from modifying the _timers array during iteration, which could make us
   -- accidently skip timers.
   if self._sv._next_timers then
      table.insert(self._sv._next_timers, timer)
   else
      table.insert(self._sv._timers, timer)
   end
   self.__saved_variables:mark_changed()

   return timer
end

return TimeTracker
