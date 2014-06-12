local AutotestInstance = require 'lib.server.autotest_instance'

local _main_thread
local _current_test_instance
local _finish_cb

local autotest_framework = {
   __saved_variables = {},
   ui = require 'lib.server.autotest_ui_server',
   env = require 'lib.server.autotest_environment',
   util = require 'lib.server.autotest_util_server',
   log = radiant.log.create_logger('autotest')
}

if autotest_framework.log:get_log_level() < radiant.log.INFO then
   autotest_framework.log:set_log_level(radiant.log.INFO)
end

local function _run_test(name, fn)
   local success

   
   autotest_framework.env.clear()

   assert(not _current_test_instance)
   _current_test_instance = AutotestInstance(name, fn)
   success = _current_test_instance:run()
   _current_test_instance = nil

   if not success then
      autotest_framework.fail('test "%s" failed.  aborting.', name)
   end
end

-- run a single autotest script
-- Loads the lua script referred to by the script parameter and runs all
-- tests for that script against the specified environment.
--
-- @param env the test environment used to run the script
-- @param script the name of the script containing the tests to run
local function _run_script(script, function_name)
   local obj = radiant.mods.load_script(script)
   assert(obj, string.format('failed to load test script "%s"', script))

   local tests = {}
   for name, fn in pairs(obj) do
      local matches = not function_name or name == function_name
      if type(fn) == 'function' and matches then
         table.insert(tests, { name = name, fn = fn })
      end
   end

   autotest_framework.log:write(0, 'running script %s', script)
   local total = #tests
   for i, test in ipairs(tests) do
      autotest_framework.log:write(0, 'running test %s (%d of %d)', test.name, i, total)
      _run_test(test.name, test.fn)
   end
end

local function _run_group(index, key)
   local group = index[key]
   if not group then
      autotest_framework.fail('undefined test group "%s"', key)
   end
   -- first run all the groups
   if group.groups then
      for i, gname in ipairs(group.groups) do
         _run_group(index, gname)
      end
   end

   -- now run all scripts for this group
   if group.scripts then
      for i, script in ipairs(group.scripts) do
         _run_script(script)
      end
   end
end

local function _run_thread(fn)
   -- xxx: move threads into radiant?
   assert(not _main_thread)
   _main_thread = stonehearth.threads:create_thread()
         :set_thread_main(function()
               fn()
               _finish_cb(0)
            end)
            
   _main_thread:start()
end


function autotest_framework.set_finish_cb(cb)
   _finish_cb = cb
end

function autotest_framework.fail(format, ...)
   local err = string.format(format, ...)
   autotest_framework.log:error(err)
   _finish_cb(1)
   if _main_thread:is_running() then
      _main_thread:terminate()
   end
   if _current_test_instance then   
      _current_test_instance:terminate()
      _current_test_instance = nil
   end
end

-- runs a set of autotest scripts
-- Runs through all the scripts in the test_scripts table.
-- @param test_scripts A numeric table containing a list of test scripts
-- to run
function autotest_framework.run_group(index, name)
   _run_thread(function()
         -- Find the correct 'world' for the specified group.
         local found_world_name = nil
         for world_name, world in pairs(index.worlds) do
            for group_name, group in pairs(world.groups) do
               if group_name == name then
                  found_world_name = world_name
                  break
               end
            end
         end
         if not found_world_name then
            autotest_framework.fail('undefined test group "%s"', name)
         end
         -- Run that group.
         _run_group(index.worlds[found_world_name].groups, name)
      end)
end

function autotest_framework.run_script(script)
   _run_thread(function()
         _run_script(script)
      end)
end

function autotest_framework.run_test(script, name)
   _run_thread(function()
         _run_script(script, name)
      end)
end

return autotest_framework
