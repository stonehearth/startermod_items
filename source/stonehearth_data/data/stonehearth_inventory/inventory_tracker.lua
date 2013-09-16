--[[
   Just like the worker tracker in the census module, this tracks all placeable items
   This means all items that have a placeable_item_proxy component
   TODO: Expand to include filters for other kinds of items, like foods
   TODO: this is SO CLOSE to the people census; can we roll them up together?
]]
local InventoryTracker = class()

function InventoryTracker:__init(data_store)
   local added_cb = function(id, entity)
      self:_on_entity_add(id, entity)
   end
   local removed_cb = function(id)
      self:_on_entity_remove(id)
   end

   self._promise = radiant.terrain.trace_world_entities('inventory tracker', added_cb, removed_cb)
   self._data = data_store:get_data()
   if not self._data.entities then
      self._data.entities = {}
   end

   self._data_store = data_store
   self._tracked_entities = {} -- used to avoid an O(n) removal for non workers

end

function InventoryTracker:_on_entity_add(id, entity)
   local placeable = entity:get_component('stonehearth_items:placeable_item_proxy')
   if placeable then
      table.insert(self._data.entities, entity)
      self._data_store:mark_changed()
      self._tracked_entities[entity:get_id()] = true
   end
end

function InventoryTracker:_on_entity_remove(id)
   if self._tracked_entities[id] then
      self._tracked_entities[id] = nil
      self._data_store:mark_changed()

      -- Handlebars can't handle (heh) associative arrays (GAH!)
      for i, entity in ipairs(self._data.entities) do
         if entity and entity:get_id() == id then
            table.remove(self._data.entities, i)
            break
         end
      end
   end
end

return InventoryTracker
