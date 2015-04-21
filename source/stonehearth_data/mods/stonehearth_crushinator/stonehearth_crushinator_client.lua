mod = {
}

local save_load = function()
   radiant.set_realtime_timer(60 * 1000, function()
         _radiant.call('radiant:client:save_game', '_crushinator',  { name = 'Crushinator save.'})
            :done(function()
                  _radiant.call('radiant:client:load_game_async', '_crushinator')
               end)
         end)
end

radiant.events.listen_once(mod, 'radiant:new_game', save_load)

radiant.events.listen_once(radiant, 'radiant:game_loaded', save_load)

return mod
