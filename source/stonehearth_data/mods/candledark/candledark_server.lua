
candledark = {
   --constants = require 'constants'
}

-- Called whenever the game starts or is loaded
radiant.events.listen(candledark, 'radiant:init', function()
   --Hack: only want this to happen once, on the first time we load. If we load after 
   --noon on the 1st day, do not create candledark
   if radiant.gamestate.now() < 194400 then
      -- HACK because we don't have module dependencies
      radiant.set_realtime_timer(1000, function()
         radiant.events.listen_once(stonehearth.calendar, 'stonehearth:noon', function()
               stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
            end)
         end)
   end
   end)
   

return candledark
