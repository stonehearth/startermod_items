
local Logger = class()

function Logger.__init(self, category, prefix)
   self._category = category
   self:set_prefix(prefix)
end

function Logger:set_prefix(prefix)
   self._prefix = prefix
end

function Logger:write(level, format, ...)
   self:_write(level, format, ...)
   return true
end

function Logger:error(format, ...)
   self:_write(radiant.log.ERROR, format, ...)
   return true
end

function Logger:warning(format, ...)
   self:_write(radiant.log.WARNING, format, ...)
   return true
end

function Logger:info(format, ...)
   self:_write(radiant.log.INFO, format, ...)
   return true
end

function Logger:debug(format, ...)
   self:_write(radiant.log.DEBUG, format, ...)
end

function Logger:spam(format, ...)
   self:_write(radiant.log.SPAM, format, ...)
end

function Logger:_write(level, format, ...)   
   if self._prefix then
      radiant.log.write(self._category, level, '[%s] '.. format, self._prefix, ...)
   else
      radiant.log.write(self._category, level, format, ...)
   end
end

return Logger
