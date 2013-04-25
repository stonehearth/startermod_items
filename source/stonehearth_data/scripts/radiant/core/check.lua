require 'unclasslib'
local util = require 'radiant.core.util'

-- replace the global assert
assert = function(condition)   
   if not condition then
      native:assert_failed('assertion failed')
   end
   return condition
end

-- xxx: break this up into the init-time and the run-time reactor?
-- perhaps by making the run-time part a singleton??

local Check = class()

function Check:is_entity(obj)
   self:verify(util:is_entity(obj))
end

function Check:is_table(obj)
   self:verify(type(obj) == 'table')
end

function Check:is_boolean(obj)
   self:verify(type(obj) == 'boolean')
end

function Check:is_string(obj)
   self:verify(type(obj) == 'string')
end

function Check:is_number(obj)
   self:verify(type(obj) == 'number')
end

function Check:is_function(obj)
   self:verify(type(obj) == 'function')
end

function Check:is_a(obj, cls)
   self:verify(util:is_a(obj, cls))
end

function Check:is_callable(obj)
   self:verify(util:is_callable(obj))
end

function Check:verify(condition)   
   if not condition then
      assert(false)
   end
end

function Check:report_error(...)
   self:report_thread_error(nil, ...)
end

function Check:report_thread_error(thread, ...)
   native:log('!! ' .. string.format('error: %s', ...))
   local msg = thread and debug.traceback(thread) or debug.traceback()
   native:log(msg)
end

function Check:contains(t, value)
   for _, v in ipairs(t) do
      if v == value then
         return
      end
   end
   assert(false)
end

return Check()
