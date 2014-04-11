local Log = {
   ERROR = 1,
   WARNING = 3,
   INFO = 5,
   DEBUG = 7,
   DETAIL = 8,
   SPAM = 9,
}

local logger_functions = {
   error = Log.ERROR,
   warning = Log.WARNING,
   info = Log.INFO,
   debug = Log.DEBUG,
   detail = Log.DETAIL,
   spam = Log.SPAM,
}

local LOG_LEVELS = {}

-- Warning!  Before reorganizaing any of the logging code, see the comment in the
-- body of write.  The stack has to be organized *just* so in order to get the
-- module name correct!  This includes those "return true"'s at the bottom of these
-- functions to disable tail-call optimization of write_.

function Log.write_(category, level, format, ...)
   if radiant.log.is_enabled(category, level) then
      local args = {...}
      for i, arg in ipairs(args) do
         if type(arg) ~= 'number' then
            args[i] = tostring(arg)
         end
      end
      _host:log(category, level, string.format(format, unpack(args)))
   end
end

function Log.get_log_level(category)
   local level = LOG_LEVELS[category]
   if level == nil then
      level = _host:get_log_level(category)
      LOG_LEVELS[category] = level
   end
   return level
end

function Log.is_enabled(category, level)
   return level <= Log.get_log_level(category)
end

function Log.write(category, level, format, ...)
   Log.write_(category, level, format, ...)
   return true
end

function Log.error(category, format, ...)
   Log.write_(category, Log.ERROR, format, ...)
   return true
end

function Log.warning(category, format, ...)
   Log.write_(category, Log.WARNING, format, ...)
   return true
end

function Log.info(category, format, ...)
   Log.write_(category, Log.INFO, format, ...)
   return true
end

function Log.debug(category, format, ...)
   Log.write_(category, Log.DEBUG, format, ...)
end

function Log.spam(category, format, ...)
   Log.write_(category, Log.SPAM, format, ...)
end

local function create_logger_functions(logger)
end

local function logger_write(self, level, format, ...)
   local prefix
   if self._prefix then
      prefix = '[' .. self._prefix .. '] '
   else
      prefix = ''
   end
   local args = {...}
   for i, arg in ipairs(args) do
      if type(arg) ~= 'string' then
         args[i] = tostring(arg)
      end
   end
   _host:log(self._category, level, prefix .. string.format(format, unpack(args)))
end

function Log.create_logger(sub_category, prefix)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: Log.create_logger       
   --    3: --> some module whose name we want! <-- 
   local category = __get_current_module_name(3) .. '.' .. sub_category
   local logger = {
      _prefix = prefix,
      _category = category,
      _log_level = Log.get_log_level(category),

      set_prefix = function (self, prefix)
            self._prefix = prefix
            return self
         end,

      is_enabled = function (self, level)
            return radiant.log.is_enabled(self._category, level)
         end,

      write = logger_write,

      get_log_level = function(self)
            return self._log_level
         end,

      set_log_level = function(self, new_level)
            for keyword, level in pairs(logger_functions) do
               if level <= new_level then
                  self[keyword] = function (l, format, ...)
                        return logger_write(l, level, format, ...)
                     end
               else
                  self[keyword] = function () end
               end
            end
            return self
         end,
   }

   logger:set_log_level(Log.get_log_level(category))   
   return logger
end

return Log
