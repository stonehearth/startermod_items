
local _timers = {}

local Timer = class()
function Timer:__init(expire_time, fn)
   self.expire_time = expire_time
   self._fn = fn
end

function Timer:destroy()
   self._destroyed = true
end

function Timer:fire()
   if not self._destroyed then
      self._fn()
   end   
end

function radiant._fire_timers()
   local now = _host:get_realtime()

   table.sort(_timers, function (l, r) return l.expire_time > r.expire_time end)
   
   local c = #_timers
   while c > 0 and _timers[c].expire_time <= now do
      local timer = table.remove(_timers)
      timer:fire()
      c = c - 1
   end
end

function radiant.get_realtime()
   return _host:get_realtime()
end

function radiant.set_realtime_timer(delay_ms, fn)
   local now = _host:get_realtime()
   local expire_time = now + (delay_ms / 1000.0)
   local timer = Timer(expire_time, fn)
   table.insert(_timers, timer)
   return timer
end

function radiant.set_performance_counter(name, value, kind)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: radiant.set_performance_counter
   --    3: --> some module whose name we want! <-- 
   local category = __get_current_module_name(3) .. ':' .. name
   if not kind then
      kind = 'counter'
   end
   _host:set_performance_counter(category, value, kind)
end

