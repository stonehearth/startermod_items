local Inventory = class()

function Inventory:__init(faction)
   self._faction = faction
end

function Inventory:initialize()
   self._data = {
      next_stockpile_no = 1,
      storage = {},
   }
   self.__savestate = radiant.create_datastore(self._data)
end

function Inventory:restore(savestate)
   self.__savestate = savestate
   self._data = savestate:get_data()
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth:stockpile')   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('stonehearth:stockpile'):set_size(size.x, size.y)

   --xxx localize
   entity:get_component('unit_info'):set_display_name('Stockpile No.' .. self._data.next_stockpile_no)
   entity:get_component('unit_info'):set_faction(self._faction)

   self._data.next_stockpile_no = self._data.next_stockpile_no + 1
   self.__savestate:mark_changed()
   return entity
end

function Inventory:add_storage(storage_entity)
   assert(not self._data.storage[storage_entity:get_id()])
   self._data.storage[storage_entity:get_id()] = {}
   self.__savestate:mark_changed()
   radiant.events.listen(storage_entity, "stonehearth:item_added",   self, self._on_item_added)
   radiant.events.listen(storage_entity, "stonehearth:item_removed", self, self._on_item_removed)
end

function Inventory:remove_storage(storage_entity)
   local id = storage_entity:get_id()
   if self._data.storage[id] then
      self._data.storage[id] = nil
      self.__savestate:mark_changed()
   end
   --xxx remove the items?
end

function Inventory:_on_item_added(e)
   local storage_id = e.storage:get_id()
   assert(self._data.storage[id], 'tried to add an item to an untracked storage entity ' .. tostring(e.storage))

   table.insert(self._data.storage[storage_id], e.item:get_id())
   self.__savestate:mark_changed()
   radiant.events.trigger(self, 'stonehearth:item_added', { item = e.item })
end

function Inventory:_on_item_removed(e)
   local storage_id = e.storage:get_id()
   assert(self._data.storage[storage_id], 'tried to remove an item to an untracked storage entity ' .. tostring(e.storage))

   for i, item in ipairs(self._data.storage[storage_id]) do
      if item == e.item:get_id() then
         table.remove(self._data.storage[storage_id], i)
         self.__savestate:mark_changed()
         radiant.events.trigger(self, 'stonehearth:item_removed', { item = e.item })
         break
      end
   end
end

return Inventory
