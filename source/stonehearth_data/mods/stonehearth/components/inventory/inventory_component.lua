local InventoryComponent = class()

function InventoryComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.items then
      self._sv.capacity = json.capacity or 4
      self._sv.items = {
         n = 0
      }
   end
end
   
function InventoryComponent:add_item(item)
   if not self:is_full() then
      table.insert(self._sv.items, item)
      self.__saved_variables:mark_changed()
   end
end

function InventoryComponent:get_items()
   return self._sv.items
end

function InventoryComponent:is_empty()
   return #self._sv.items == 0
end

function InventoryComponent:is_full()
   return #self._sv.items == self._sv.capacity
end

function InventoryComponent:remove_first_item()
   local item = nil

   if not self:is_empty() then
      item = table.remove(self._sv.items, 1)
      self.__saved_variables:mark_changed()
   end
   
   return item
end

return InventoryComponent
