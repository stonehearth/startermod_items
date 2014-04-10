
-- Runs a single function in an autotest suite
local AutotestInstance = class()
local AutotestUtil = require 'lib.server.autotest_util_server'

function AutotestInstance:__init(name, fn)
   self._fn = fn
   self._name = name

   self._log = radiant.log.create_logger('autotest')
                                 :set_prefix(name)
   if self._log:get_log_level() < radiant.log.INFO then
      self._log:set_log_level(radiant.log.INFO)
   end

   self.util = AutotestUtil(self)
   self:_protect_framework_interface('ui')
   self:_protect_framework_interface('env')
end

function AutotestInstance:_protect_framework_interface(iname)
   local protected_interface = {}

   local interface = autotest_framework[iname]
   for fname, fn in pairs(interface) do
      if fname[1] ~= '_' then
         protected_interface[fname] = function (_, ...)
            self:_check_running(iname .. ':' .. fname)
            return fn(...)
         end
      end
   end
   self[iname] = protected_interface
end

function AutotestInstance:_check_running(fn_name)
   if self._thread:is_finished() then
      local err  = string.format('call to autotest:%s() from %s when thread is not running', fn_name, self._name)
      self._log:error(err)
      autotest_framework.fail(err)
   end
end

function AutotestInstance:log(format, ...) 
   self:_check_running('log')
   self._log:info(format, ...)
end

function AutotestInstance:fail(format, ...)
   self:_check_running('fail')
   if format then
      local err = string.format(format, ...)
      self._log:error("%s failed: %s", self._name, err)
   end
   self._fail_called = true
   self._thread:terminate()
end

function AutotestInstance:success()
   self:_check_running('success')
   self._success_called = true
   self._thread:terminate()
end

function AutotestInstance:suspend()
   self:_check_running('suspend')
   assert(stonehearth.threads:get_current_thread() == self._thread)
   self._thread_suspended = true
   self._thread:suspend()
end

function AutotestInstance:resume()
   self:_check_running('resume')
   assert(self._thread_suspended)
   if self._thread then
      self._thread:resume()
      self._thread_suspended = false
   end
end

function AutotestInstance:sleep(ms)
   self:_check_running('sleep')
   local wakeup = radiant.get_realtime() + (ms / 1000.0)
   repeat
      radiant.set_realtime_timer(ms, function()
            if not self._thread:is_finished() then
               self._thread:resume()
            end
         end)
      self._thread:suspend()
   until self._thread:is_finished() or radiant.get_realtime() >= wakeup
end

function AutotestInstance:terminate()
   if not self._thread:is_finished() then
      self._thread:terminate()
   end
end

function AutotestInstance:run()
   -- we create a new thread here so we can call :terminate() as soon
   -- as the test function calls :success() or :fail()
   self._thread = stonehearth.threads:create_thread()
                     :set_thread_main(function()
                           self._fn(self)
                        end)

   self._thread:start()
   self._thread:wait()
   return self._success_called and not self._fail_called
end

return AutotestInstance
