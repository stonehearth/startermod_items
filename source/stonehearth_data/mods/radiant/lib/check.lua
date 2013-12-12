local check = {}

function check.is_entity(obj)
   return obj and
          obj.get_type_name and
          obj:get_type_name() == 'class radiant::om::Entity' and
          obj:is_valid()
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

function check.is_a(value, class)
   return classof(value) and value:is_a(class) or false
end

function check.verify(condition)   
   if not condition then
      assert(false)
   end
end

function check.report_error(...)
   check.report_thread_error(nil, ...)
end

function check.report_thread_error(thread, format, ...)   
   local error = string.format(format, ...)
   local traceback = thread and debug.traceback(thread) or debug.traceback()
   _host:report_error(error, traceback)
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
