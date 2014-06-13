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

local function _run_group(all_groups, group)
   -- first run all the groups
   if group.groups then
      for i, gname in ipairs(group.groups) do
         local subgroup = all_groups.groups[gname]
         if not subgroup then
            autotest_framework.fail('unknown group name "%s"', gname)
         end
         _run_group(all_groups, subgroup)
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

-- run a grouop of test scripts.  it's up to the client to ensure  that the world has been setup correctly
-- `group` must be a table containing the following keys:
--
--     'groups' : an optional list of group names.  these will be used to index into the `all_groups`
--                table to find the group objects to execute recursively.
--
--     'scripts' : a list of .lua files containing the tests to execute (see run_script)
--
--    @param all_groups - an table containing key/value pairs for the recursive subgroups.
--    @param group - a table containing the group to run
--
function autotest_framework.run_group(all_groups, group)
   _run_thread(function()
         _run_group(all_groups, group)
      end)
end

-- run a specific test script.  it's up to the client to ensure  that the world has been setup correctly
--
--    @param script - the .lua script file with the tests
--    @param function_name - the name of the specific function to run.   if omitted, all functions
--                            in `script` will be executed.
--
function autotest_framework.run_script(script, function_name)
   _run_thread(function()
         _run_script(script, function_name)
      end)
end

return autotest_framework
