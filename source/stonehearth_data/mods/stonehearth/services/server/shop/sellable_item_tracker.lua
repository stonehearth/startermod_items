local SellableItemTracker = class()

function SellableItemTracker:initialize()
   self._sv._entity_id_to_uri = {}
   self._sv._iconic_id_to_root_id = {}
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
      return radiant.entities.get_category(entity)
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
   if not tracking_data then
      tracking_data = {}
   end

   local uri = entity:get_uri()
   local id = entity:get_id()
   local net_worth = radiant.entities.get_entity_data(entity, 'stonehearth:net_worth')
   local item_cost = net_worth.value_in_gold

   if not tracking_data[uri] then
      local unit_info = entity:add_component('unit_info')      
      tracking_data[uri] = {
         uri = uri,
         items = {},
         icon = unit_info:get_icon(),
         display_name = unit_info:get_display_name(),
         icon = unit_info:get_icon(),
      }
   end

   tracking_data[uri].items[id] = entity

   if not self._sv._entity_id_to_uri[id] then
      self._sv._entity_id_to_uri[id] = uri
      self.__saved_variables:mark_changed()
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
   local root_id = self._sv._iconic_id_to_root_id[entity_id]
   local uri = self._sv._entity_id_to_uri[root_id]
   if uri then 
      self._sv._entity_id_to_uri[root_id] = nil
      
      tracking_data[uri].items[root_id] = nil
      if not next(tracking_data[uri]) then
         tracking_data[uri] = nil
      end

      self.__saved_variables:mark_changed()
   end
   return tracking_data
end

return SellableItemTracker
