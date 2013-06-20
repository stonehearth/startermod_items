local resources = {}

local proxy_metatable = {
   __index = function (proxy, key)
      return resources._wrap(proxy.__obj:get(key))
   end,
   __newindex = function (proxy, key, value)
      error('json resource has no entry for ' .. key)
   end,
   __tostring = function (proxy)
      return tostring(proxy.__obj)
   end,
   __towatch = function (proxy)
      return tostring(proxy.__obj)
   end
}

function resources.__init()
end

function resources._wrap(obj)
   if type(obj) ~= 'userdata' then
      return obj
   end
   local proxy = { 
      __is_radiant_json = true,
      __obj = obj
   }
   setmetatable(proxy, proxy_metatable)
   return proxy
end

function resources.load_animation(uri)
   return native:load_animation(uri)
end

function resources.load_json(uri)
   return resources._wrap(native:load_json(uri))
end

function resources.get_native_obj(proxy)
   return proxy.__obj
end

function resources.len(proxy)
   return proxy.__obj:len()
end

function resources.pairs(proxy)
   local f, s, var = proxy.__obj:contents()
   local fproxy = function (s, var)
      local key, value = f(s, var)
      return key, resources._wrap(value)
   end
   return fproxy, s, var
end

resources.__init()
return resources
