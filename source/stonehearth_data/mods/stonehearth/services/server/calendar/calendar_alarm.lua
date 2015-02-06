local CalendarAlarm = class()

function CalendarAlarm:__init(time, fn)
   self._fn = fn
   self._time = time
   self:reset()
end

function CalendarAlarm:reset()
   if self._is_dead then
      return
   end
   local expire_time = self._time
   if type(expire_time) == 'string' then
      expire_time = stonehearth.calendar:parse_time(expire_time)
   end
   assert(expire_time >= 0, string.format('invalid time "%s"', tostring(self._time)))
   
   self._expire_time = expire_time
end

function CalendarAlarm:destroy()
   self._is_dead = true
end

function CalendarAlarm:is_dead()
   return self._is_dead
end

function CalendarAlarm:fire()
   assert(not self._is_dead)
   self._fn()
end

function CalendarAlarm:get_expire_time()
   return self._expire_time
end

return CalendarAlarm

