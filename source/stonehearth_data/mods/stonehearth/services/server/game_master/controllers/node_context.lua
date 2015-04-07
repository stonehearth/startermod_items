
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
      for k, v in pairs(parent_ctx) do
         if not self[k] then 
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
-- Sample strings: create_camp.npc_boss_entity, create_camp.entities.wolf_1
-- @param path - dot-separated path that can be found inside ctx
-- @param deflt - optional! Ff we can't find anything, return the default value instead of nil
-- @returns the value if found, nil if not, default if not found and default is provided
function NodeContext:get(path, deflt)
   local path_arr = radiant.util.split_string(path, '.')
   local ctx_ptr = self
   for i, piece in ipairs(path_arr) do 
      if ctx_ptr[piece] then
         ctx_ptr = ctx_ptr[piece]
      else
         ctx_ptr = nil
         break
      end
   end
   if deflt and not ctx_ptr then
      ctx_ptr = deflt
   end
   return ctx_ptr
end

return NodeContext._create_factory()
