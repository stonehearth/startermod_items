mod = {
}

local save_load_trigger = function()
   radiant.set_realtime_timer(10 * 1000, function()
         _radiant.call('radiant:client:save_game', '_crushinator',  { name = 'Crushinator save.'})
            :done(function()
                  _radiant.call('radiant:client:load_game_async', '_crushinator')
               end)
         end)
end

radiant.events.listen_once(mod, 'radiant:new_game', function()
      mod.__saved_variables:get_data().init = true
      mod.__saved_variables:mark_changed()
      save_load_trigger()
   end)


radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
      if mod.__saved_variables:get_data().init then
         save_load_trigger()
      end
   end)

return mod
