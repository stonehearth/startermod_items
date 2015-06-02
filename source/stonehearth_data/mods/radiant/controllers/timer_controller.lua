local log = radiant.log.create_logger('timer')
local Timer = class()

function Timer:initialize(reason, start_time, duration, fn, repeating)
   checks('self', 'string', 'number', 'number', 'table|function', '?boolean')
   assert(fn)
   self._sv._active = true
   self._sv._start_time = start_time
   self._sv._duration = duration
   self._sv._expire_time = start_time + duration
   self._sv._repeating = repeating
   self._sv._reason = reason
   self._sv._fn = fn

   log:debug('Timer is intialized: start time: %f, expires: %f', self._sv._start_time, self._sv._expire_time)
end

function Timer:destroy()
   self._sv._active = false
   self.__saved_variables:mark_changed()
end

function Timer:bind(fn)
   self._sv._fn = fn
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
   if not self._sv._fn then
      radiant.log.write('timer', 0, 'timer %s with duration %d has no callback.  killing.', self._sv._reason, self._sv._duration)
      self._sv._active = false
      return
   end
   self.__saved_variables:mark_changed()   
   
   if type(self._sv._fn) == 'function' then
      self._sv._fn()
   else
      radiant.invoke(self._sv._fn)
   end
end

return Timer
