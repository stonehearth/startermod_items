
stonehearth_tests = {
}

radiant.events.listen(stonehearth_tests, 'radiant:new_game', function(args)
      local script = radiant.util.get_config('test', 'harvest_test')
      require(script)()
   end)

return stonehearth_tests
