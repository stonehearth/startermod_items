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

function util.equals(value)
end

function util.tostring(value)
   local t = type(value)
   if t == 'userdata' then
      local v = tostring(value)
      if true then
         return v
      end
      if value.get_type_name then
         return value:get_type_name()
      end
      return class_info(value).name
   elseif util.is_class(value) then
      return value.__classname or '*anonymous class*'
   elseif util.is_instance(value) then
      return util.tostring(value.__class)
   end
   return tostring(value)
end

function util.is_instance(maybe_instance)
   return type(maybe_instance) == 'table' and maybe_instance.__type == 'object'
end

function util.is_class(maybe_cls)
   return type(maybe_cls) == 'table' and maybe_cls.__type == 'class'
end

function util.table_is_empty(t)
   return #t == 0 and next(t) == nil
end

function util.is_a(var, cls)
   local t = type(var)
   if t == 'userdata' then
      return var:get_type_id() == cls.get_type_id()
   elseif util.is_instance(var) and util.is_class(cls) then
      return var:is_a(cls)
   end
   return t == cls
end

function util.get_config(str, default)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: util.get_config       
   --    3: --> some module whose name we want! <-- 
   local modname = __get_current_module_name(3)
   local value = _host:get_config('mods.' .. modname .. "." .. str)
   return value == nil and default or value
end

return util
