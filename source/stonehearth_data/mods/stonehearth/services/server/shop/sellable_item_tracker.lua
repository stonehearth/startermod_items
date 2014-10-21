local SellableItemTracker = class()

function SellableItemTracker:initialize()
   -- nothing to do
end

function SellableItemTracker:restore()
   -- nothing to do
end

-- Part of the inventory tracker interface.  Given an `entity`, return the key
-- used to store tracker data for entities of that type.  Most of the
-- time this is just the entity uri, but if the item is actually the 'iconic'
-- version of a larger entity, it's full entity's uri, not the iconic's uri.
--
--    @param entity - the entity currently being tracked
-- 
function SellableItemTracker:create_key_for_entity(entity)
   assert(entity:is_valid(), 'entity is not valid.')


   -- is it sellable?
   local net_worth = radiant.entities.get_entity_data(entity, 'stonehearth:net_worth')
   if net_worth and net_worth.shop_info and net_worth.shop_info.sellable then
      local uri = entity:get_uri()
      return uri
   end

   -- nope!
   return nil

end

-- Part of the inventory tracker interface.  Add an `entity` to the `tracking_data`.
-- Tracking data is the existing data stored for entities sharing the same key as
-- `entity` (see :create_key_for_entity()).  We store both an array of all entities
-- sharing this uri and a total count.
--
--     @param entity - the entity being added to tracking data
--     @param tracking_data - the tracking data for all entities of the same type
--1
function SellableItemTracker:add_entity_to_tracking_data(entity, tracking_data)

   local uri = entity:get_uri()
   local net_worth = radiant.entities.get_entity_data(entity, 'stonehearth:net_worth')
   local item_cost = net_worth.value_in_gold

   if not tracking_data then
      -- We're the first object of this type.  Create a new tracking data structure.
      tracking_data = {
         uri = entity:get_uri(), 
         item = entity,
         cost = item_cost,
         num = 1,
      }
   else 
      tracking_data.num = tracking_data.num + 1
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
function SellableItemTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   -- If for some reason, there's no existing value for this key, just return
   if not tracking_data then
      return nil
   end

   if tracking_data.items[entity_id] then
      tracking_data.items[entity_id] = nil
      tracking_data.num = tracking_data.num - 1
      assert(tracking_data.count >= 0)
   end

   return tracking_data
end

return SellableItemTracker
