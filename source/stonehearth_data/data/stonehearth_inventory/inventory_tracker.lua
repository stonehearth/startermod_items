--[[
   Just like the worker tracker in the census module, this tracks all placeable items
   This means all items that have a placeable_item_proxy component.
   TODO: Expand to include filters for other kinds of items, like foods
   TODO: this is SO CLOSE to the people census; can we roll them up together?
   TODO: Sort by object attributes (wooden, food, etc?)
   TODO: What about materials? Can we use this as the inventory for the crafter?
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
   if not self._data.entity_types then
      self._data.entity_types = {}
   end
   self._data.num_entities = 0;

   self._data_store = data_store
   self._tracked_entities = {} -- used to avoid an O(n) removal for non workers
   self._tracked_identifiers = {}  --map identifiers to array of entities of that identifiers
end

--[[
   Entities are sorted into buckets by type. Each entity type object contains
   identifying data about that entity (name, icon, material, id, etc) and an
   array of entities of that type. We also keep a table to access those
   inner arrays, so that we have fast access when we need to add/remove stuff.
]]
function InventoryTracker:_on_entity_add(id, entity)
   local placeable = entity:get_component('stonehearth_items:placeable_item_proxy')
   local item_info = entity:get_component('item')
   local unit_info = entity:get_component('unit_info')

   if placeable and item_info then

      self._data.num_entities = self._data.num_entities + 1

      local identifier = item_info:get_identifier()
      local category = item_info:get_category()

      --array of entities that are of the same type
      local entities_of_this_type = self._tracked_identifiers[identifier]

      if entities_of_this_type then
         table.insert(entities_of_this_type, entity)
      else
         entities_of_this_type = {}
         table.insert(entities_of_this_type, entity)
         self._tracked_identifiers[identifier] = entities_of_this_type

         local new_entity_data = {
            entity_name = unit_info:get_display_name(),
            entity_icon = unit_info:get_icon(),
            entity_category = item_info:get_category(),
            entity_identifier = item_info:get_identifier(),
            entities = entities_of_this_type
         }
         table.insert(self._data.entity_types, new_entity_data)
      end

      self._data_store:mark_changed()
      self._tracked_entities[entity:get_id()] = true
   end
end

function InventoryTracker:_on_entity_remove(id)
   if self._tracked_entities[id] then
      self._data.num_entities = self._data.num_entities - 1
      self._tracked_entities[id] = nil
      self._data_store:mark_changed()

      local target_entity = radiant.entities.get_entity(id)
      local identifier = target_entity:get_component('item'):get_identifier()
      local entities_of_this_type = self._tracked_identifiers[identifier]

      -- Handlebars can't handle (heh) associative arrays (GAH!)
      for i, entity in ipairs(entities_of_this_type) do
         if entity and entity:get_id() == id then
            table.remove(entities_of_this_type, i)
            break
         end
      end
   end
end

return InventoryTracker
