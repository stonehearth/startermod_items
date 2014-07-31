local EquipmentTracker = class()
--[[
   Holds the functions used by the basic inventory tracker
   Packaged as a controller so the inventory tracker has easy access to them on creation
]]

function EquipmentTracker:initialize()
   -- nothing to do
end

function EquipmentTracker:restore()
   -- nothing to do
end

-- Part of the inventory tracker interface.  Given an `entity`, return the key
-- used to store tracker data for entities of that type.  Since we're just counting
-- the entities with the same uri, this is just the entity uri.
--
--    @param entity - the entity currently being tracked
-- 
function EquipmentTracker:create_key_for_entity(entity)
   local full_sized, equipment_piece = radiant.entities.unwrap_iconic_item(entity, 'stonehearth:equipment_piece')
   if equipment_piece then
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
function EquipmentTracker:add_entity_to_tracking_data(entity, tracking_data)
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
function EquipmentTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   return nil
end

return EquipmentTracker
