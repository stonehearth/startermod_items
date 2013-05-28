local check = {}

function check.is_entity(obj)
   check.verify(radiant.util.is_entity(obj))
end

function check.is_table(obj)
   check.verify(type(obj) == 'table')
end

function check.is_boolean(obj)
   check.verify(type(obj) == 'boolean')
end

function check.is_string(obj)
   check.verify(type(obj) == 'string')
end

function check.is_number(obj)
   check.verify(type(obj) == 'number')
end

function check.is_function(obj)
   check.verify(type(obj) == 'function')
end

function check.is_a(obj, cls)
   check.verify(radiant.util.is_a(obj, cls))
end

function check.is_callable(obj)
   check.verify(radiant.util.is_callable(obj))
end

function check.verify(condition)   
   if not condition then
      assert(false)
   end
end

function check.report_error(...)
   check.report_thread_error(nil, ...)
end

function check.report_thread_error(thread, ...)
   native:log('!! ' .. string.format('error: %s', ...))
   local msg = thread and debug.traceback(thread) or debug.traceback()
   native:log(msg)
end

function check.contains(t, value)
   for _, v in ipairs(t) do
      if v == value then
         return
      end
   end
   assert(false)
end

return check
