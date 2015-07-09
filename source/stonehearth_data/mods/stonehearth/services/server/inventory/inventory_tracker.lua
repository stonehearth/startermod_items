local InventoryTracker = class()

--[[
   A service can create a filtered tracker to mantain a set of key/value pair tracking_data.
   The service is then responsible for updating the appropriate filtered trackers when
   it hears that the objects in them might be changed.

   The service takes a controller containing a bunch of relevant functions.
]]

--- Init the InventoryTracker
--  @param controller - a controller with various functions needed for the filter. 
--
function InventoryTracker:initialize(controller)
   assert(controller)
   self._sv.controller = controller
   self._sv._ids_to_keys = {}
   self._sv.tracking_data = {}
end

function InventoryTracker:restore()
   -- nothing to do
end

--- Call when it's time to add an item
--
function InventoryTracker:add_item(entity, storage)
   local controller = self._sv.controller
   local key = controller:create_key_for_entity(entity, storage)

   -- A 'nil' key means ignore the entity.
   if key ~= nil then
      -- save the key for this entity for the future, so we know how to remove the entity
      -- from our tracking tracking_data when it's destroyed.      
      local id = entity:get_id()
      self._sv._ids_to_keys[id] = key

      -- Get existing value from key and give it to the controller so it can generate the
      -- next value
      local tracking_data = self._sv.tracking_data[key]
      self._sv.tracking_data[key] = controller:add_entity_to_tracking_data(entity, tracking_data)
      self.__saved_variables:mark_changed()

      radiant.events.trigger(self, 'stonehearth:inventory_tracker:item_added:sync', { key = key, item = entity })
      radiant.events.trigger_async(self, 'stonehearth:inventory_tracker:item_added', { key = key })
   end
end

--- Call when it's time to remove an item
--
function InventoryTracker:remove_item(entity_id)
   local key = self._sv._ids_to_keys[entity_id]
   if key then
      self._sv._ids_to_keys[entity_id] = nil

      local controller = self._sv.controller
      local tracking_data = self._sv.tracking_data[key]
      self._sv.tracking_data[key] = controller:remove_entity_from_tracking_data(entity_id, tracking_data)
      self.__saved_variables:mark_changed()
      
      radiant.events.trigger(self, 'stoneheahrth:inventory_tracker:item_removed:sync', { key = key, item_id = entity_id })
      radiant.events.trigger_async(self, 'stonehearth:inventory_tracker:item_removed', { key = key })
   end
end

--If the item is not in us, re-evaluate it for add.
--If it is in us, re-evaluate it for remove
function InventoryTracker:reevaluate_item(entity, storage)
   local entity_id = entity:get_id()
   if self._sv._ids_to_keys[entity_id] then
      --the item is already being tracked. Should it be removed?
      local key = self._sv.controller:create_key_for_entity(entity, storage)
      if not key then
         self:remove_item(entity_id)
      end
   else
      --the item is not being tracked. Try to add it. 
      --It will only get added, if it fits the tracker (ie, if a key can be generated for it)
      self:add_item(entity, storage)
   end
end


--The item or it's container has changed. 
--function InventoryTracker:update_item_container(item, storage)
--   local controller = self._sv.controller
--   if controller.update_item_container then
--      controller:update_item_container(item, storage)
--   end
--end

-- Returns the tracking data
--
function InventoryTracker:get_tracking_data()
   return self._sv.tracking_data
end

function InventoryTracker:trace(reason)
   return self.__saved_variables:trace(reason)
end

function InventoryTracker:mark_changed()
   return self.__saved_variables:mark_changed()
end

return InventoryTracker
