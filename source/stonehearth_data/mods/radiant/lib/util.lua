local util = {}

--[[
  Was failing whenever the object was a RadiantI3 because obj:is_valid() was undefined.
]]

function util.is_string(obj)
   return type(obj) == 'string'
end

function util.is_number(obj)
   return type(obj) == 'number'
end

function util.colorcode_to_integer(cc)
   local result = 0
   if type(cc) == 'string' then
      -- parse the digits out of 'rgb(13, 34, 19)', allowing for spaces between tokens
      local r, g, b = cc:match('rgb%s*%(%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*%)')
      if r and g and b then
         result = (((tonumber(b) * 256) + tonumber(g)) * 256) + tonumber(r)
      end
   end
   return result
end

function util.get_config(str, default)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: util.config_get       
   --    3: --> some module whose name we want! <-- 
   local modname = __get_current_module_name(3)
   local value = _host:get_config('mods.' .. modname .. "." .. str)
   return value == nil and default or value
end

return util
