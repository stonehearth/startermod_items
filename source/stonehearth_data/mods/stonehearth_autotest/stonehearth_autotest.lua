mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
      autotest.run_tests(index.test_groups.all)
      radiant.exit(0)
   end)

return mod
