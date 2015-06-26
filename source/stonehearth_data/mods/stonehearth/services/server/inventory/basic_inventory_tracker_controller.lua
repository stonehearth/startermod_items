local BasicInventoryTracker = class()
--[[
   Holds the functions used by the basic inventory tracker
   Packaged as a controller so the inventory tracker has easy access to them on creation
]]

function BasicInventoryTracker:initialize()
   -- nothing to do
end

function BasicInventoryTracker:restore()
   -- nothing to do
end

-- Part of the inventory tracker interface.  Given an `entity`, return the key
-- used to store tracker data for entities of that type. This has to always be
-- the entity uri, even if it is iconic. Otherwise, crafter maintain will break.
--
--    @param entity - the entity currently being tracked
-- 
function BasicInventoryTracker:create_key_for_entity(entity)
   assert(entity:is_valid(), 'entity is not valid.')
   local uri = entity:get_uri()
   return uri
end

-- Part of the inventory tracker interface.  Add an `entity` to the `tracking_data`.
-- Tracking data is the existing data stored for entities sharing the same key as
-- `entity` (see :create_key_for_entity()).  We store both an array of all entities
-- sharing this uri and a total count.
--
--     @param entity - the entity being added to tracking data
--     @param tracking_data - the tracking data for all entities of the same type
--1
function BasicInventoryTracker:add_entity_to_tracking_data(entity, tracking_data)
   if not tracking_data then
      -- We're the first object of this type.  Create a new tracking data structure.

      local unit_info = entity:add_component('unit_info')
      tracking_data = {
         uri = entity:get_uri(),
         count = 0,
         items = {},
         icon = unit_info:get_icon(),
         display_name = unit_info:get_display_name(),
         description = unit_info:get_description(),
         category = radiant.entities.get_category(entity),
         first_item = entity,
      }
   end

   -- Add the current entity to the tracking data, and return
   local id = entity:get_id()
   if not tracking_data.items[id] then
      tracking_data.count = tracking_data.count + 1
      tracking_data.items[id] = entity
   end
   return tracking_data
end

-- Part of the inventory tracker interface.  Remove the entity with `entity_id` from
-- the `tracking_data`.  Tracking data is the existing data stored for entities sharing 
-- the same key as (see :create_key_for_entity()).
--
--    @param entity_id - the entity id of the thing being removed.
--    @param tracking_data - the tracking data for all entities of the same type
--
function BasicInventoryTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   -- If for some reason, there's no existing value for this key, just return
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

return BasicInventoryTracker
