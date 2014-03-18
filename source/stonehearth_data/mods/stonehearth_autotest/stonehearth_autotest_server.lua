mod = {}

radiant.events.listen(mod, 'radiant:new_game', function(args)
      radiant.events.listen(autotest, 'autotest:finished', function(result)
            radiant.exit(result.errorcode)
         end)

      autotest.env.create_world()

      radiant.set_realtime_timer(3000, function()
         local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
         local options = radiant.util.get_config('options', {})
         if options.script then
            if options['function'] then
               autotest.run_function(options.script, options['function'])
            else
               autotest.run_script(options.script)
            end
         elseif options.group then         
            autotest.run_group(index, options.group)
         else
            autotest.run_group(index, 'all')
         end
         --autotest.run_script('stonehearth_autotest/tests/end_to_end/sleep_autotests.lua')
      end)
   end)

return mod
