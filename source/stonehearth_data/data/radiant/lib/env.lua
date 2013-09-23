-- replace the global assert
assert = function(condition)   
   if not condition then
      _host:assert_failed('assertion failed')
   end
   return condition
end

-- this function is only valid in very specific circumstances!  specfically, the
-- caller must be called DIRECTLY from a 3rd party module source file.
__get_current_module_name = function()
   -- level 1 would be this function, __get_current_module_name in env.lua...
   -- level 2 would be the caller of this function (e.g. require in env.lua...)
   -- level 3 is the caller of the caller, which is in the module we're looking for!
   local info = debug.getinfo(3, 'S')

   if not info.source then
      radiant.log.warning('could not determine module file in radiant "require"')
      return nil
   end
   if info.source:sub(1, 1) ~= '@' then
      radiant.log.warning('lua generated from loadstring() is not allowed to require.')
      return nil
   end
   local modname = info.source:match('@data[/\\]([^/\\]*)')
   if not modname then
      radiant.log.warning('could not determine modname from source "%s"', info.source)
      return nil
   end
   return modname
end

local old_require = require   -- hide the lua implementation
require = function(s)
   local modname = __get_current_module_name()
   if modname then
      return _host:require(modname .. '.' ..s)
   end
end


require 'lualibs.unclasslib'

-- finish off by locking down the global namespace
require 'lualibs.strict'
