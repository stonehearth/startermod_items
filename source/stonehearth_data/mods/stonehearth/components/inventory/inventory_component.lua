local InventoryComponent = class()

function InventoryComponent:initialize(entity, json)
   self._entity = entity
   self._items = {
      __numeric = true
   }
   self._capacity = json.capacity or 4
   self.__saved_variables = radiant.create_datastore({
         items = self._items,
         capacity = self._capacity,
      })
end
   
function InventoryComponent:add_item(item)
   if not self:is_full() then
      table.insert(self._items, item)
      self.__saved_variables:mark_changed()
   end
end

function InventoryComponent:get_items()
   return self._items
end

function InventoryComponent:is_empty()
   return #self._items == 0
end

function InventoryComponent:is_full()
   return #self._items == self._capacity
end

function InventoryComponent:remove_first_item()
   local item = nil

   if not self:is_empty() then
      item = table.remove(self._items, 1)
      self.__saved_variables:mark_changed()
   end
   
   return item
end

return InventoryComponent
