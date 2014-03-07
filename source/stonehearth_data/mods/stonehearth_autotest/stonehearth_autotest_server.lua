mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      radiant.events.listen(autotest, 'autotest:finished', function(result)
            radiant.exit(result.errorcode)
         end)

      radiant.set_timer(3000, function()
         local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
         autotest.run_group(index, 'all')
         --autotest.run_script('stonehearth_autotest/tests/end_to_end/construction_autotests.lua')
         --autotest.run_script('stonehearth_autotest/tests/end_to_end/carpenter_autotests.lua')
         --autotest.run_script('stonehearth_autotest/tests/end_to_end/food_autotests.lua')
      end)
   end)

return mod
