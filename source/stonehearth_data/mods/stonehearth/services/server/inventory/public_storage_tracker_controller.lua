local PublicStorageTracker = class()
--[[
   Tracks everything in public storage
   This means stuff that is added with a storage component, and whose storage is public
]]

function PublicStorageTracker:initialize()
end

function PublicStorageTracker:restore()
end

-- Part of the inventory tracker interface
-- Given an entity, return the key used to store tracker data for entities of that type
-- If the entity does not currently belong to public storage, return nil
-- @param entity: the entity to add
-- @param storage: optional, the entity that this item is being put into, nil if it's being taken out, or not applicable
function PublicStorageTracker:create_keys_for_entity(entity, storage)
   assert(entity:is_valid(), 'entity is not valid.')
   local uri = entity:get_uri()
   if storage then
      local storage_component = storage:get_component('stonehearth:storage') 
      if storage_component and storage_component:is_public() then
         return {uri}
      end
   end
   return nil
end

--Given an entity and the existing tracking data for that entity, add the entity properly to the tracking data
--If there is no tracking data, create a new tracking data object.
function PublicStorageTracker:add_entity_to_tracking_data(entity, tracking_data)
   if not tracking_data then
      --create the tracking data for this type of object
      local unit_info = entity:add_component('unit_info')
      tracking_data = {
         uri = entity:get_uri(),
         count = 0, 
         items = {},
         first_item = entity
      }
   end

   local id = entity:get_id()
   if not tracking_data.items[id] then
      tracking_data.count = tracking_data.count + 1
      tracking_data.items[id] = entity
   end
   return tracking_data   
end

-- Part of the inventory tracker interface.  Remove the entity with `entity_id` from
-- the `tracking_data`. 
function PublicStorageTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   if not tracking_data then
      return nil
   end

   if tracking_data.items[entity_id] then
      tracking_data.items[entity_id] = nil
      tracking_data.count = tracking_data.count - 1

      if tracking_data.count <= 0 then
         tracking_data.first_item = nil
         return nil
      end

      local id, next_item = next(tracking_data.items)
      tracking_data.first_item = next_item
   end
   return tracking_data
end

return PublicStorageTracker