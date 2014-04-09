mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      radiant.events.listen(autotest_framework, 'autotest:finished', function(result)
            radiant.exit(result.errorcode)
         end)

      autotest_framework.env.create_world()

      radiant.set_realtime_timer(3000, function()
         local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
         local options = radiant.util.get_config('options', {})
         if options.script then
            if options['function'] then
               autotest_framework.run_function(options.script, options['function'])
            else
               autotest_framework.run_script(options.script)
            end
         elseif options.group then         
            autotest_framework.run_group(index, options.group)
         else
            autotest_framework.run_group(index, 'all')
         end
         --autotest_framework.run_script('stonehearth_autotest/tests/end_to_end/sleep_autotests.lua')
      end)
   end)

return mod
