mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      radiant.events.listen(autotest, 'autotest:finished', function(result)
            radiant.exit(result.errorcode)
         end)

      radiant.set_timer(3000, function()
         local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
         autotest.run_tests(index, 'all')
      end)
   end)

return mod
