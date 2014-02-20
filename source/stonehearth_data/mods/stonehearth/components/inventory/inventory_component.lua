local InventoryComponent = class()

function InventoryComponent:__init(entity, data_binding)
   self._entity = entity
   self._item_entities = {}
   self._data_binding = data_binding
   
   self._data = {}
   self._data.items = self._item_entities
   self._data.capacity = 4 --xxx hardcoded

   self._data_binding:update(self._data)
end
   
function InventoryComponent:add_item(item)
   if not self:is_full() then
      table.insert(self._item_entities, item)
      self._data_binding:update(self._data)
   end
end

function InventoryComponent:get_items()
   return self._item_entities
end

function InventoryComponent:is_empty()
   return #self._item_entities == 0
end

function InventoryComponent:is_full()
   return #self._item_entities == self._data.capacity
end

function InventoryComponent:remove_first_item()
   local item = nil

   if not self:is_empty() then
      item = table.remove(self._item_entities, 1)
      self._data_binding:update(self._data)
   end
   
   return item
end

return InventoryComponent
