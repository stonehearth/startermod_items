
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
   end
   rawset(self, k, v)
end

function NodeContext:__next(k)
   return next(self._sv, k)
end

function NodeContext:initialize(parent_ctx)
   self.child_nodes = {}
   if parent_ctx then
      for k, v in pairs(parent_ctx) do
         if not self[k] then 
            self[k] = v
         end
      end
      table.insert(parent_ctx.child_nodes, self)
      parent_ctx:mark_changed()
   end
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

return NodeContext._create_factory()
