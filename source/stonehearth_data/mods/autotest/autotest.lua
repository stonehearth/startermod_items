local TestEnvironment = require 'lib.test_environment'
local TestRunner = require 'lib.test_runner'

local _all_tests = {}

local autotest = {
   __savestate = {}
}

function autotest.log(format, ...)
   radiant.log.write('autotest', 0, format, ...)
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

-- run a single autotest script
-- Loads the lua script referred to by the script parameter and runs all
-- tests for that script against the specified environment.  Each test
-- in the script is simply a function that gets passed the an (env, runner)
-- tuple (see TestEnvionment and TestRunner)
--
-- @param env the test environment used to run the script
-- @param script the name of the script containing the tests to run
function autotest.run_script(env, script)
   local obj = radiant.mods.load_script(script)
   assert(obj, string.format('failed to load test script "%s"', script))

   local tests = {}
   for name, fn in pairs(obj) do
      if type(fn) == 'function' then
         table.insert(tests, { name = name, fn = fn })
      end
   end

   local total = #tests
   for i, test in ipairs(tests) do
      autotest.log('running test %s (%d of %d)', test.name, i, total)
      local success, err = TestRunner():run(env, test.fn)
      if not success then
         error(err)
      end
   end
end

-- runs a set of autotest scripts
-- Runs through all the scripts in the test_scripts table.
-- @param test_scripts A numeric table containing a list of test scripts
-- to run
function autotest.run_tests(test_scripts)
   -- xxx: move threads into radiant?
   stonehearth.threads:create_thread()
         :set_thread_main(function()
               local env = TestEnvironment()
               for i, script in ipairs(test_scripts) do
                  autotest.run_script(env, script)
               end
            end)
         :start()
end

return autotest
