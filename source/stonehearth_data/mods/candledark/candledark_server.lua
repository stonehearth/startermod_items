
candledark = {}

-- Listen for the 'radiant:init' event, which gets triggered whenever the game starts or is loaded
--
radiant.events.listen(candledark, 'radiant:init', function()
   -- We'd like to start four in-game hours after the game starts, but the Stonehearth scenario 
   -- system isn't sophisticated enough to handle that yet.  Until that happy day, just wait until 
   -- noon and force the scenario to start.

   -- We only want this to happen once, on the first time we load. If we load after 
   -- noon on the 1st day, do not create candledark
   --
   local ticks_per_second = 9 
   if radiant.gamestate.now() < 60 * 60 * 4 * ticks_per_second then
      radiant.set_realtime_timer(1000, function()
         stonehearth.calendar:set_timer('4h', function()
               stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark')
            end)
         end)
   end
   end)
   

return candledark