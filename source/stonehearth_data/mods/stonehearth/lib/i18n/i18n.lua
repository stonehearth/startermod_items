local Entity = _radiant.om.Entity

-- internationalization library
--
local i18n = class()

function i18n:__init()
   self._helpers = {}
   
   -- a helper to render the display name of an entity
   -- usage:  %name(boss)
   --
   self:add_helper('name', { Entity }, function(e)
         return radiant.entities.get_display_name(e)
      end)
end

-- performs helper substition on a string.  we assume the string was loaded from some i18n'ed resource.
--
function i18n:format_string(input, context)
   local result = ''
   while true do
      -- find the next format block
      local epos = input:find('%%')
      if not epos then
         return result .. input
      end
      -- tack the prefix onto the result 
      result = result .. input:sub(1, epos - 1)
      input  = input:sub(epos)

      -- capture the name of the argument and everything between the call
      local name, args = input:match('%%(%w+)%(([^%)]*)%)')
      if not name or not args then
         return input
      end

      -- skip and continue
      local call_length = #name + #args + 4
      result = result .. self:_call_helper(name, args, context)
      input  =  input:sub(call_length)
   end   
end

-- add a new helper to the library
--
function i18n:add_helper(name, args, fn)
   self._helpers[name] = {
      fn = fn,
      args = args,
   }
end

-- call as registered helper
--
function i18n:_call_helper(name, args, context)
   local helper = self._helpers[name]
   if not helper then
      return string.format('no i18n helper named %%%s')
   end

   args = args:split(',')
   for i, arg in pairs(args) do
      local arg_stripped = arg:strip()
      local value = context:get(arg_stripped)
      if not value then
         return string.format('i18n helper %%%s: could not find "%s" in arg %d', name, arg, i)
      end
      if not radiant.util.is_a(value, helper.args[i]) then
         return string.format('i18n helper %%%s: arg %d "%s" has wrong type', name, i, arg)
      end
      args[i] = value
   end
   return helper.fn(unpack(args))
end

return i18n
