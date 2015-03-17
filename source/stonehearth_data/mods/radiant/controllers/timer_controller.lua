TimeTracker = class()
local Timer = class()

function Timer:initialize(start_time, duration, fn, repeating)
   assert(fn)
   self._sv._active = true
   self._sv._start_time = start_time
   self._sv._duration = duration
   self._sv._expire_time = start_time + duration
   self._sv._repeating = repeating
   self._fn = fn
end

function Timer:destroy()
   self._sv._active = false
   self.__saved_variables:mark_changed()
end

function Timer:bind(fn)
   self._fn = fn
end

function Timer:is_active()
   return self._sv._active
end


function Timer:get_expire_time()
   return self._sv._expire_time
end

function Timer:fire(now)
   if self._sv._repeating then
      self._sv._expire_time = now + self._sv._duration
   else
      self._sv._active = false
   end
   if not self._fn then
      radiant.log.write('timer', 0, 'timer with duration %d has no callback.  killing.', self._sv._duration)
      self._sv._active = false
      return
   end
   self.__saved_variables:mark_changed()   
   
   self._fn()
end

return Timer
