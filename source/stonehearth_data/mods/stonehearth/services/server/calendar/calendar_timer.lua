local CalendarTimer = class()

function CalendarTimer:__init(start_time, duration, fn, repeating)
   self._active = true
   self._start_time = start_time
   self._duration = duration
   self._expire_time = start_time + duration
   self._repeating = repeating
   self._fn = fn
end

function CalendarTimer:destroy()
   self._active = false
end

function CalendarTimer:is_active()
   return self._active
end

--This way you can get the new expire time during the callback, if so desired
function CalendarTimer:fire()
   if self._repeating then
      self._expire_time = self._expire_time + self._duration
   else
      self._active = false
   end
   self._fn()
end

function CalendarTimer:get_expire_time()
   return self._expire_time
end

function CalendarTimer:get_remaining_time()
   return self:get_expire_time() - stonehearth.calendar:get_elapsed_game_time()
end

return CalendarTimer
