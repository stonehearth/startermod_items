mod = {}


function parse_options(index, options)
   local run_world, run_group, run_script

   local function endswith(s, suffix)
      return s:sub(-suffix:len()) == suffix
   end

   -- if a group is specified, use that.  otherwise, if a script is specified, use that.
   -- if neither is specified, use the 'all' group.
   if options.group then
      options.script = nil
   elseif not options.script then
      options.group = 'all'
   end

   -- find the world which matches the script or group chosen
   for world_name, world in pairs(index.worlds) do
      for group_name, group in pairs(world.groups) do
         if options.group and group_name == options.group then
            -- the name of this group matches the one we were looking for.
            -- run all tests from this group and this world.
            return world, group, nil
         end

         if options.script and group.scripts then
            for _, script in ipairs(group.scripts) do
               if endswith(script, options.script) then
                  -- the name of this script matches the one we were looking for.
                  -- run all tests from this script and this world.
                  return world, nil, script
               end
            end
         end
      end
   end
end

radiant.events.listen(mod, 'radiant:new_game', function(args)
      local index = radiant.resources.load_json('stonehearth_autotest/tests/index.json')
      local options = radiant.util.get_config('options', {})
      local world, group, script = parse_options(index, options)

      if not world then
         radiant.log.write('stonehearth_autotest', 0, 'could not determine world script.  aborting.')
         radiant.exit(1)
      end
      if not group and not script then
         radiant.log.write('stonehearth_autotest', 0, 'could not determine group or script to execute.  aborting.')
         radiant.exit(1)
      end

      autotest_framework.set_finish_cb(function(errorcode)
            radiant.exit(errorcode)
         end)

      autotest_framework.env.create_world(world.world_generator)

      radiant.set_realtime_timer(3000, function()
         if group then
            local all_groups = world
            autotest_framework.run_group(all_groups, group)
         elseif script then
            autotest_framework.run_script(script, options['function'])
         end
      end)
   end)

return mod
