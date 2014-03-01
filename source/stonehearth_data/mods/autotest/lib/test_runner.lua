local TestRunner = class()

-- Run a single test in the specified environment
-- The test_fn parameter should be a function which takes the TestEnvironment,
-- and TestRunner.  For example:
--
--       function (env, runner)
--          env:create_entity(0, 0, 'stonehearth:oak_log')
--          runner:success()
--       end
-- 
-- @param the environment to run the test in
-- @param test_fn the test to run.
function TestRunner:run(env, test_fn)
   -- initialize a default error message in case the test is completely
   -- buggy
   self._success = false
   self._error = 'test failed to call :success() or :fail()'
   
   -- we create a new thread here so we can call thread:terminate() as soon
   -- as the test function calls :success() or :fail()
   self._thread = stonehearth.threads:create_thread()
                     :set_thread_main(function()
                           test_fn(env, self)
                        end)
                        
   self._thread:start()
   self._thread:wait()
   return self._success, self._error
end

-- Report that a test has completed succesfully
function TestRunner:success()
   self._success = true
   self._thread:terminate()
end

function TestRunner:fail(format, ...)
   self._success = false
   self._error = string.format(format, ...)
   self._thread:terminate()
end

function TestRunner:sleep(ms)
   local wakeup = radiant.gamestate.now() + ms
   repeat
      radiant.set_timer(wakeup, function()
            if not self._thread:is_finished() then
               self._thread:resume()
            end
         end)            
      self._thread:suspend()
   until self._thread:is_finished() or radiant.gamestate.now() >= wakeup
end

return TestRunner
