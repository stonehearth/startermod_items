local Log = class()

function Log:info(format, ...)
   self:_write('', format, ...)
end

function Log:debug(format, ...)
   -- self:_write('..', format, ...)
end

function Log:warning(format, ...)
   self:_write('!!', format, ...)
end

function Log:_write(prefix, format, ...)
   native:log(string.format('%8d : %2s %s', env and env:now() or 0, prefix, string.format(format, ...)))
end

return Log()
