
local Logger = class()

function Logger.__init(self, category)
   self._category = category
end

function Logger:write(level, format, ...)
   radiant.log.write_(self._category, level, format, ...)
   return true
end

function Logger:error(format, ...)
   radiant.log.write_(self._category, radiant.log.ERROR, format, ...)
   return true
end

function Logger:warning(format, ...)
   radiant.log.write_(self._category, radiant.log.WARNING, format, ...)
   return true
end

function Logger:info(format, ...)
   radiant.log.write_(self._category, radiant.log.INFO, format, ...)
   return true
end

function Logger:debug(format, ...)
   radiant.log.write_(self._category, radiant.log.DEBUG, format, ...)
end

function Logger:spam(format, ...)
   radiant.log.write_(self._category, radiant.log.SPAM, format, ...)
end

return Logger
