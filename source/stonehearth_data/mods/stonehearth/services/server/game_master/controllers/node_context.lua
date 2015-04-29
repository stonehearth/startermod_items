
local NodeContext = {}

function NodeContext:__index(k)
   local mt = getmetatable(self)
   if mt[k] then 
      return mt[k]
   end
   return self._sv[k]
end

function NodeContext:__newindex(k, v)
   if k ~= '_sv' and k ~= '__saved_variables' then
      self._sv[k] = v
      self.__saved_variables:mark_changed()
      return
   end
   rawset(self, k, v)
end

function NodeContext:__next(k)
   return next(self._sv, k)
end

function NodeContext:initialize(node, parent_ctx)
   if parent_ctx then
      for k, v in pairs(parent_ctx._sv) do
         -- don't overwrite keys that already exist and don't copy the
         -- parent node pointer (we'll save our own node in at the end of
         -- the function)
         if not self[k] and k ~= 'node' then 
            self[k] = v
         end
      end
   end
   self.node = node
end

function NodeContext:get_data()
   return self._sv
end

function NodeContext:mark_changed()
   self.__saved_variables:mark_changed()
end

function NodeContext._create_factory()
   return function()
      local instance = {}
      setmetatable(instance, NodeContext)
      return instance
   end
end

-- Given a string path to a value, split it and find it in the ctx
-- Sample strings: create_camp.boss, foo.bar[1].baz
-- @param path - dot-separated path that can be found inside ctx
-- @param deflt - optional! Ff we can't find anything, return the default value instead of nil
-- @returns the value if found, nil if not, default if not found and default is provided
function NodeContext:get(path, deflt)
   -- path is something like "foo.bar[1].baz".  transform to
   -- "self.foo.bar[1].baz" and eval and we're all good!

   -- create a function that we can pass a paramter to to look up `path`
   local fn = string.format('return function(self) return self.%s end', path)
   local f, error = loadstring(fn)
   if f == nil then
      -- parse error?  no problem!
      return deflt
   end
   
   -- now call that function, passing ourself in!
   local success, result = pcall(function()
         local fetch = f()
         return fetch(self)
      end)
   if not success then
      return deflt
   end
   
   return result
end

return NodeContext._create_factory()
