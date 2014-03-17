local _success
local _all_tests = {}
local _test_thread
local _test_thread_suspended
local _main_thread

local autotest = {
   __saved_variables = {},
   ui = require 'lib.server.autotest_ui_server',
   env = require 'lib.server.autotest_environment',
   util = require 'lib.server.autotest_util_server',
}

function autotest.log(format, ...) 
   radiant.log.write('autotest', 0, format, ...)
end

function autotest.fail(format, ...)
   local e = format and string.format(format, ...) or 'no error provided'
   autotest.log(e)
   radiant.events.trigger(autotest, 'autotest:finished', { errorcode = 1 })
   _test_thread:terminate()
   _main_thread:terminate()
end

function autotest.success()
   _success = true
   _test_thread:terminate()
end

function autotest.suspend()
   assert(stonehearth.threads:get_current_thread() == _test_thread)
   _test_thread_suspended = true
   _test_thread:suspend()
end

function autotest.resume()
   assert(_test_thread_suspended)
   if _test_thread then
      assert(not _test_thread:is_finished())
      _test_thread:resume()
      _test_thread_suspended = false
   end
end

function autotest.sleep(ms)
   assert(stonehearth.threads:get_current_thread() == _test_thread)

   local wakeup = radiant.gamestate.now() + ms
   repeat
      local thread = _test_thread
      radiant.set_timer(wakeup, function()
            if not thread:is_finished() then
               thread:resume()
            end
         end)
      thread:suspend()
   until thread:is_finished() or radiant.gamestate.now() >= wakeup
end

function autotest.check_table(expected, actual, route)
   local parent_route = route or ''
   if #parent_route > 0 then
      parent_route = parent_route .. '.'
   end
   for name, value in pairs(expected) do
      local route = parent_route .. name
      local val = actual[name]
      if not val then
         return false, string.format('expected "%s" value "%s" not in table', route, tostring(name), tostring(value))
      end
      if type(val) ~= type(value) then
         return false, string.format('expected "%s" type "%s" does not match actual type "%s"', route, type(value), type(val))
      end
      if type(val) == 'table' then
         local success, err = autotest.check_table(value, val, route)
         if not success then
            return false, err
         end
      else
         if val ~= value then
            return false, string.format('expected "%s" value "%s" does not match actual value "%s"', route, value, val)
         end      
      end
   end
   return true
end

local function _run_test(fn)
   assert(not _test_thread, 'tried to run test before another finished!')

   -- we create a new thread here so we can call thread:terminate() as soon
   -- as the test function calls :success() or :fail()
   _test_thread = stonehearth.threads:create_thread()
                     :set_thread_main(function()
                           fn()
                        end)
                        
   _success = false
   autotest.env.clear()
   _test_thread:start()
   _test_thread:wait()
   _test_thread = nil
   if not _success then
      autotest.fail('test failed to call :success() or :fail()')
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

   autotest.log('running script %s', script)
   local total = #tests
   for i, test in ipairs(tests) do
      autotest.log('running test %s (%d of %d)', test.name, i, total)
      _run_test(test.fn)
   end
end

local function _run_group(index, key)
   local group = index[key]
   if not group then
      autotest.fail('undefined test group "%s"', key)
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
               radiant.events.trigger(autotest, 'autotest:finished', { errorcode = 0 })
            end)
            
   _main_thread:start()
end

-- runs a set of autotest scripts
-- Runs through all the scripts in the test_scripts table.
-- @param test_scripts A numeric table containing a list of test scripts
-- to run
function autotest.run_group(index, name)
   _run_thread(function()
         _run_group(index.groups, name)
      end)
end

function autotest.run_script(script)
   _run_thread(function()
         _run_script(script)
      end)
end

function autotest.run_test(script, name)
   _run_thread(function()
         _run_script(script, name)
      end)
end

return autotest
