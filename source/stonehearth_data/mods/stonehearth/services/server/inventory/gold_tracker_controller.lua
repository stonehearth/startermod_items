local GoldTracker = class()

function GoldTracker:initialize()
   -- nothing to do
end

function GoldTracker:restore()
   -- nothing to do
end

-- Part of the inventory tracker interface.  Given an `entity`, return the key
-- used to store tracker data for entities of that type.  Since we're just counting
-- the entities with the same uri, this is just the entity uri.
--
--    @param entity - the entity currently being tracked
-- 
function GoldTracker:create_key_for_entity(entity)
   local uri = entity:get_uri()
   if uri == 'stonehearth:gold_container' then
      return entity:get_id()
   end
end

-- Part of the inventory tracker interface.  Add an `entity` to the `tracking_data`.
-- Tracking data is the existing data stored for entities sharing the same key as
-- `entity` (see :create_key_for_entity()).  We store both an array of all entities
-- sharing this uri and a total count.
--
--     @param entity - the entity being added to tracking data
--     @param tracking_data - the tracking data for all entities of the same type
--
function GoldTracker:add_entity_to_tracking_data(entity, tracking_data)
   assert(tracking_data == nil or tracking_data == entity)
   return entity
end

-- Part of the inventory tracker interface.  Remove the entity with `entity_id` from
-- the `tracking_data`.  Tracking data is the existing data stored for entities sharing 
-- the same key as (see :create_key_for_entity()).
--
--    @param entity_id - the entity id of the thing being removed.
--    @param tracking_data - the tracking data for all entities of the same type
--
function GoldTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   return nil
end

return GoldTracker
