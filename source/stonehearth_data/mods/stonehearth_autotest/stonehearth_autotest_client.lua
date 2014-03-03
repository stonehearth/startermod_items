mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      autotest.ui.connect()
   end)

return mod
