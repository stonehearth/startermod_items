
candledark = {
   --constants = require 'constants'
}

radiant.events.listen(candledark, 'radiant:init', function()
      -- HACK because we don't have module dependencies
      radiant.set_realtime_timer(1000, function()
         radiant.events.listen_once(stonehearth.calendar, 'stonehearth:noon', function()
               stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
            end)
         end)
   end)

return candledark
