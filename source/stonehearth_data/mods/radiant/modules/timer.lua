
local _timers = {}

function radiant._fire_timers()
   local now = radiant.gamestate.now()

   table.sort(_timers, function (l, r) return l.time > r.time end)
   
   local c = #_timers
   while c > 0 and _timers[c].time <= now do
      local timer = table.remove(_timers)
      timer.fn()
      c = c - 1
   end
end

function radiant.set_timer(time, fn)
   local now = radiant.gamestate.now()
   table.insert(_timers, {
         time = time,
         fn = fn
      })
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
