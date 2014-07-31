local PlaceableItemsView = class()
--[[
   Used to provide view-specific data for the place items view (see place_item.js)
]]

function PlaceableItemsView:initialize()
   self._sv._entity_id_to_uri = {}
   -- nothing to do
end

function PlaceableItemsView:restore()
   -- nothing to do
end

-- Part of the inventory tracker interface.  Given an `entity`, return the key
-- used to store tracker data for entities of that type.  Group items by category.
--
--    @param entity - the entity currently being tracked
-- 
function PlaceableItemsView:create_key_for_entity(entity)
   local placable_component = entity:get_component('stonehearth:iconic_form')
   if not placable_component then
      return nil
   end
   return placable_component:get_category()
end

-- Part of the inventory tracker interface.  Add an `entity` to the `tracking_data`.
-- Tracking data is the existing data stored for entities sharing the same key as
-- `entity` (see :create_key_for_entity()).  We store both an array of all entities
-- sharing this uri and a total count.
--
--     @param entity - the entity being added to tracking data
--     @param tracking_data - the tracking data for all entities of the same type
--
function PlaceableItemsView:add_entity_to_tracking_data(entity, tracking_data)   
   local placable_component = entity:get_component('stonehearth:iconic_form')
   if not tracking_data then
      tracking_data = {}
   end
   local uri = entity:get_uri()
   if not tracking_data[uri] then
      local unit_info = entity:add_component('unit_info')      
      tracking_data[uri] = {
         count = 0,
         icon = unit_info:get_icon(),
         display_name = unit_info:get_display_name(),
      }
   end
   tracking_data[uri].count = tracking_data[uri].count + 1

   local id = entity:get_id()
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
function PlaceableItemsView:remove_entity_from_tracking_data(entity_id, tracking_data)
   local uri = self._sv._entity_id_to_uri[entity_id]
   if uri then
      self._sv._entity_id_to_uri = nil
      self.__saved_variables:mark_changed()
      
      tracking_data[uri].count = tracking_data[uri].count - 1
      if tracking_data[uri].count == 0 then
         tracking_data[uri] = nil
      end
   end
   return tracking_data
end

return PlaceableItemsView
