
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
