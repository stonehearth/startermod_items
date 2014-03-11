local Inventory = class()

function Inventory:__init(faction)
   self._faction = faction
   self._nextStockpileNo = 1;
   self._storage = {}
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth:stockpile')   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('stonehearth:stockpile'):set_size(size.x, size.y)
   --xxx localize
   entity:get_component('unit_info'):set_display_name('Stockpile No.' .. self._nextStockpileNo)
   entity:get_component('unit_info'):set_faction(self._faction)

   self._nextStockpileNo = self._nextStockpileNo + 1
   return entity
end

function Inventory:add_storage(storage_entity)
   assert(not self._storage[storage_entity:get_id()])
   self._storage[storage_entity:get_id()] = {}
   radiant.events.listen(storage_entity, "stonehearth:item_added",   self, self.on_item_added)
   radiant.events.listen(storage_entity, "stonehearth:item_removed", self, self.on_item_removed)
end

function Inventory:get_storage_entities()
   local storage = {}
   for id, _ in pairs(self._storage) do
      local entity = radiant.entities.get_entity(id)
      table.insert(storage, entity)
   end

   return storage
end

function Inventory:get_item_entities()
   local items = {}
   for id, _ in pairs(self._storage) do
      for item_id, _ in pairs(self._storage[id]) do
         local entity = radiant.entities.get_entity(self._storage[id][item_id])
         table.insert(items, entity)
      end
   end

   return items
end

function Inventory:remove_storage(storage_entity)
   self._storage[storage_entity:get_id()] = nil
   --xxx remove the items?
end

function Inventory:on_item_added(e)
   assert(self._storage[e.storage:get_id()], 'tried to add an item to an untracked storage entity ' .. tostring(e.storage))

   table.insert(self._storage[e.storage:get_id()], e.item:get_id())
   radiant.events.trigger(self, 'stonehearth:item_added', { item = e.item })
end

function Inventory:on_item_removed(e)
   local storage_id = e.storage:get_id()
   assert(self._storage[storage_id], 'tried to remove an item to an untracked storage entity ' .. tostring(e.storage))

   for i, item in ipairs(self._storage[storage_id]) do
      if item == e.item:get_id() then
         table.remove(self._storage[storage_id], i)
         radiant.events.trigger(self, 'stonehearth:item_removed', { item = e.item })
         break
      end
   end
end

return Inventory
