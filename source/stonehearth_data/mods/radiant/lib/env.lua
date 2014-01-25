-- replace the global assert
local old_assert = assert
assert = function(condition, msg)
   if not condition then
      old_assert(condition, msg)
   end
end

-- if these variables don't exist yet, they never will.  put them in the global
-- namespace so we can check them before using (otherwise strict.lua will kick
-- us in the teeth just for checking against nil)
if not decoda_break_on_error then
   decoda_break_on_error = false
end
if not decoda_output then
   decoda_output = false
end

-- this function is only valid in very specific circumstances!  specfically, the
-- caller must be called DIRECTLY from a 3rd party module source file.
__get_current_module_name = function(depth)
   local info = debug.getinfo(depth, 'S')

   if not info.source then
      _host:log('could not determine module file in radiant "require"')
      return nil
   end
   if info.source:sub(1, 1) ~= '@' then
      _host:log('lua generated from loadstring() is not allowed to require.')
      return nil
   end
   local modname = info.source:match('@([^/\\]*)')
   if not modname then
      modname = info.source:match('@\\.[/\\]([^/\\]*)')
   end
   if not modname then
      _host:log(string.format('could not determine modname from source "%s"', info.source))
      return nil
   end
   return modname
end

local old_require = require   -- hide the lua implementation
require = function(s)
   -- check standard lua stuff..
   local o = package.loaded[s]
   if o then
      return o
   end

   -- level 1 would be the __get_current_module_name function in env.lua...
   -- level 2 would be the caller of this function (e.g. require in env.lua...)
   -- level 3 is the caller of the caller, which is in the module we're looking for!
   local modname = __get_current_module_name(3)
   if modname then
      return _host:require(modname .. '.' ..s)
   end
end

require 'lualibs.unclasslib'
