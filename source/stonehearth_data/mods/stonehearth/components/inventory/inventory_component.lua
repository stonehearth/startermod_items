local InventoryComponent = class()

function InventoryComponent:__create(entity, json)
   self._entity = entity
   self._item_entities = {}
   self._capacity = json.capacity or 4
   self.__save_state = radiant.create_datastore({
         items = self._item_entities,
         capacity = self._capacity,
      })
end
   
function InventoryComponent:add_item(item)
   if not self:is_full() then
      table.insert(self._item_entities, item)
      self.__save_state:mark_changed()
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
      self.__save_state:mark_changed()
   end
   
   return item
end

return InventoryComponent
