local Logger = require 'modules.logger'

local Log = {
   ERROR = 1,
   WARNING = 3,
   INFO = 5,
   DEBUG = 7,
   SPAM = 9,
}

local LOG_LEVELS = {}

-- Warning!  Before reorganizaing any of the logging code, see the comment in the
-- body of write.  The stack has to be organized *just* so in order to get the
-- module name correct!  This includes those "return true"'s at the bottom of these
-- functions to disable tail-call optimization of write_.

function Log.write_(category, level, format, ...)
   local config_level = LOG_LEVELS[category]
   if config_level == nil then
      config_level = _host:get_log_level(category)
      LOG_LEVELS[category] = config_level
   end
   
   if level <= config_level then
      local args = {...}
      for i, arg in ipairs(args) do
         if type(arg) == 'userdata' then
            args[i] = tostring(arg)
         end
      end
      _host:log(category, level, string.format(format, unpack(args)))
   end
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

function Log.create_logger(sub_category)
   -- The stack offset for the helper functions is 4...
   --    1: __get_current_module_name
   --    2: Log.create_logger       
   --    3: --> some module whose name we want! <-- 
   local category = __get_current_module_name(3) .. '.' .. sub_category
   return Logger(category)
end

return Log
