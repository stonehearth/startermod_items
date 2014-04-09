mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      autotest_framework.ui.connect()
   end)

return mod
