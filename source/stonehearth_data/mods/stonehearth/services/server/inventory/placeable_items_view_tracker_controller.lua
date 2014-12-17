local PlaceableItemsView = class()
--[[
   Used to provide view-specific data for the place items view (see place_item.js)
]]

local function get_root_entity(iconic_entity)
   local iconic_form = iconic_entity:get_component('stonehearth:iconic_form')
   if not iconic_form then
      return nil
   end
   return iconic_form:get_root_entity()
end

function PlaceableItemsView:initialize()
   self._sv._entity_id_to_uri = {}
   self._sv._iconic_id_to_root_id = {}
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
   local root_entity = get_root_entity(entity)
   if not root_entity then
      return nil
   end

   local entity_forms = root_entity:get_component('stonehearth:entity_forms')
   if not entity_forms then
      return nil
   end
   if not entity_forms:is_placeable() then
      return nil
   end

   local iconic_entity = entity_forms:get_iconic_entity()
   return iconic_entity:get_uri()
end

function PlaceableItemsView:add_entity_to_tracking_data(iconic_entity, tracking_data)
   local entity = get_root_entity(iconic_entity)
   
   self._sv._iconic_id_to_root_id[iconic_entity:get_id()] = entity:get_id()

   if not tracking_data then
      -- We're the first object of this type.  Create a new tracking data structure.

      local unit_info = entity:add_component('unit_info')      
      tracking_data = {
         uri = entity:get_uri(),
         count = 0, 
         items = {},
         icon = unit_info:get_icon(),
         display_name = unit_info:get_display_name(),
         category = radiant.entities.get_category(entity),
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
function PlaceableItemsView:remove_entity_from_tracking_data(entity_id, tracking_data)
   -- If for some reason, there's no existing value for this key, just return
   local root_id = self._sv._iconic_id_to_root_id[entity_id]

   if not tracking_data then
      return nil
   end

   if tracking_data.items[root_id] then
      tracking_data.items[root_id] = nil
      tracking_data.count = tracking_data.count - 1
      assert(tracking_data.count >= 0)
   end
   return tracking_data
end

return PlaceableItemsView
