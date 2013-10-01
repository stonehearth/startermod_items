local Log = {}

local _write = function(prefix, format, ...)
   _host:log(string.format('%8d : %2s %s', 0, prefix, string.format(format, ...)))
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
