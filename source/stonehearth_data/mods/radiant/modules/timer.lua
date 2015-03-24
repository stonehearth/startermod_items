local log = radiant.log.create_logger('timer')

local last_system_time = nil

radiant.events.listen(radiant, 'stonehearth:gameloop', function()
      local system_time = _host:get_realtime()

      if last_system_time == nil then
         last_system_time = system_time
      end

      local delta = system_time - last_system_time
      radiant._sv.time_tracker:increment_now(delta)
      last_system_time = system_time

      log:spam('now: %f', radiant.get_realtime())
   end)

-- client and server times may differ slightly!
function radiant.get_realtime()
   -- returns seconds, not milliseconds!
   return radiant._sv.time_tracker:get_now()
end

function radiant.set_realtime_timer(delay_ms, fn)
   return radiant._sv.time_tracker:set_timer(delay_ms / 1000, fn)
end

function radiant.set_realtime_interval(delay_ms, fn)
   return radiant._sv.time_tracker:set_interval(delay_ms / 1000, fn)
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

