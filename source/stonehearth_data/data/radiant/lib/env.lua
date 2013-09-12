-- replace the global assert
assert = function(condition)   
   if not condition then
      native:assert_failed('assertion failed')
   end
   return condition
end

local old_require = require
require = function(s)
   local info = debug.getinfo(2, 'S')

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
   package.path = string.format('data/%s/?.lua', modname)
   return old_require(s)
end


require 'lualibs.unclasslib'

-- finish off by locking down the global namespace
require 'lualibs.strict'
