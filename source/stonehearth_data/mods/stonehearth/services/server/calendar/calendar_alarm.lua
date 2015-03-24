local CalendarAlarm = class()

function CalendarAlarm:initialize(time, fn)
   self._fn = fn
   self._sv.time = time
   self:reset()
end

function CalendarAlarm:bind(fn)
   self._fn = fn
end

function CalendarAlarm:reset()
   if self._sv.is_dead then
      return
   end
   local expire_time = self._sv.time
   if type(expire_time) == 'string' then
      expire_time = stonehearth.calendar:parse_time(expire_time)
   end
   assert(expire_time >= 0, string.format('invalid time "%s"', tostring(self._sv.time)))
   
   self._sv.expire_time = expire_time
   self.__saved_variables:mark_changed()
end

function CalendarAlarm:destroy()
   self._sv.is_dead = true
   self.__saved_variables:mark_changed()
end

function CalendarAlarm:is_dead()
   return self._sv.is_dead or not self._fn
end

function CalendarAlarm:fire()
   assert(not self._sv.is_dead)
   if not self._fn then
      radiant.log.write('calendar', 0, 'alarm for time %s has no callback.  killing.', tostring(self._sv.time))
      self:destroy()
      return
   end
   self._fn()
end

function CalendarAlarm:get_expire_time()
   return self._sv.expire_time
end

return CalendarAlarm

