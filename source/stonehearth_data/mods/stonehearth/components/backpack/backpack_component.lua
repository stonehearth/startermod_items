local BackpackComponent = class()

function BackpackComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   
   if not self._sv.initialized then
      self._sv.items = {}
      self._sv.num_items = 0
      self._sv.capacity = json.capacity or 99
      self._sv.initialized = true
   end
end
   
function BackpackComponent:add_item(item)
   if self:is_full() then
      return false
   end

   local id = item:get_id()

   if not self._sv.items[id] then
      self._sv.items[id] = item
      self._sv.num_items = self._sv.num_items + 1
      self.__saved_variables:mark_changed()
   end

   return true
end

function BackpackComponent:remove_item(item)
   local id = item:get_id()

   if self._sv.items[id] then
      self._sv.items[id] = nil
      self._sv.num_items = self._sv.num_items - 1
      self.__saved_variables:mark_changed()
      return true
   end

   return false
end

function BackpackComponent:contains_item(item)
   for id, backpack_item in pairs(self._sv.items) do
      if backpack_item == item then
         return true
      end
   end
   
   return false
end

function BackpackComponent:get_items()
   return self._sv.items
end

function BackpackComponent:num_items()
   return self._sv.num_items
end

function BackpackComponent:capacity()
   return self._sv.capacity
end

function BackpackComponent:is_empty()
   return self._sv.num_items == 0
end

function BackpackComponent:is_full()
   return self._sv.num_items >= self._sv.capacity
end

function BackpackComponent:remove_first_item()
   local id, item = next(self._sv.items)

   if item then
      self:remove_item(item)
   end
   
   return item
end

return BackpackComponent
