mod = {}


function world_from_options(index, options)
   if options.script then
      local script_entry = 'file(' + options.script + ')'
      for _, world in ipairs(index.worlds) do
         for _, group in ipairs(world.groups) do
            for _, script in ipairs(group.scripts) do
               if script == script_entry then
                  return world
               end
            end
         end
      end
      return 'tiny'
   end

   local group = options.group
   if not group then
      group = 'all'
   end

   for _, world in ipairs(index.worlds) do
      for _, group in ipairs(world.groups) do
         if group == options.group then
            return world
         end
      end
   end

   return 'tiny'
end

radiant.events.listen(mod, 'radiant:new_game', function(args)
      local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
      local options = radiant.util.get_config('options', {})
      local world_kind = world_from_options(index, options)
      local world_script = index.worlds[world_kind].world_generator
      autotest_framework.env.create_world(world_script)

      autotest_framework.set_finish_cb(function(errorcode)
            radiant.exit(errorcode)
         end)

      radiant.set_realtime_timer(3000, function()
         if options.script then
            if options['function'] then
               autotest_framework.run_test(options.script, options['function'])
            else
               autotest_framework.run_script(options.script)
            end
         elseif options.group then         
            autotest_framework.run_group(index, options.group)
         else
            autotest_framework.run_group(index, 'all')
         end
      end)
   end)

return mod
