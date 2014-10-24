
candledark = {}

-- Listen for the 'radiant:init' event, which gets triggered whenever the game starts or is loaded
--
radiant.events.listen(candledark, 'radiant:init', function()
   -- We'd like to start on noon of the first day, but the Stonehearth scenario system isn't
   -- sophisticated enough to handle that yet.  Until that happy day, just wait until noon
   -- and force the scenario to start.

   -- We only want this to happen once, on the first time we load. If we load after 
   -- noon on the 1st day, do not create candledark
   --
   if radiant.gamestate.now() < 194400 then
      radiant.set_realtime_timer(1000, function()
         radiant.events.listen_once(stonehearth.calendar, 'stonehearth:noon', function()
               stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
            end)
         end)
   end
   end)
   

return candledark
