local entity_forms = require 'stonehearth.lib.entity_forms.entity_forms_lib'

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
function SellableItemTracker:create_keys_for_entity(entity, storage)
   assert(entity:is_valid(), 'entity is not valid.')

   local entity_uri, _, _ = entity_forms.get_uris(entity)
   local sellable 

   -- is it sellable?
   local net_worth = radiant.entities.get_entity_data(entity_uri, 'stonehearth:net_worth')
   if net_worth and net_worth.shop_info and net_worth.shop_info.sellable then
      sellable = true
   end

   --if it's sellable, AND it is public storage or escrow storage, then return the uri as the key
   if sellable and storage then
      local storage_component = storage:get_component('stonehearth:storage') 
      if storage_component and (storage_component:is_public() or storage_component:get_name() == 'escrow') then
         return {entity:get_uri()}
      end
   end

   -- otherwise, nope!
   return nil

end

-- Part of the inventory tracker interface.  Add an `entity` to the `tracking_data`.
-- Tracking data is the existing data stored for entities sharing the same key as
-- `entity` (see :create_keys_for_entity()).  We store both an array of all entities
-- sharing this uri and a total count.
--
--     @param entity - the entity being added to tracking data
--     @param tracking_data - the tracking data for all entities of the same type
--1
function SellableItemTracker:add_entity_to_tracking_data(entity, tracking_data)
   if not tracking_data then
      -- We're the first object of this type.  Create a new tracking data structure.

      local unit_info = entity:add_component('unit_info')      
      local entity_uri, _, _ = entity_forms.get_uris(entity)
      local cost = radiant.entities.get_entity_data(entity_uri, 'stonehearth:net_worth').value_in_gold
      local resale = math.ceil(cost * stonehearth.constants.shop.RESALE_CONSTANT)

      tracking_data = {
         uri = entity:get_uri(),
         count = 0, 
         items = {},
         icon = unit_info:get_icon(),
         display_name = unit_info:get_display_name(),
         category = radiant.entities.get_category(entity),
         cost = cost,
         resale = resale
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
-- the same key as (see :create_keys_for_entity()).
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
      tracking_data.count = tracking_data.count - 1
      
      if tracking_data.count <= 0 then
         return nil
      end
   end
   return tracking_data
end


return SellableItemTracker
