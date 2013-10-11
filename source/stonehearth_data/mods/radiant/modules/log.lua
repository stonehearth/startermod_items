local Log = {}

local _write = function(prefix, format, ...)
   local args = {...}
   for i, arg in ipairs(args) do
      if type(arg) == 'userdata' then
         args[i] = tostring(arg)
      end
   end
   _host:log(string.format('%8d : %2s %s', 0, prefix, string.format(format, unpack(args))))
end

function Log.info(format, ...)
   _write('', format, ...)
end

function Log.debug(format, ...)
   --_write('..', format, ...)
end

function Log.warning(format, ...)
   _write('!!', format, ...)
end


return Log
