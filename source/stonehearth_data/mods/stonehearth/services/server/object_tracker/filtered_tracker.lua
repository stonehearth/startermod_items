local FilteredTracker = class()

--[[
   A service can create a filtered tracker to mantain a set of key/value pair tracking_data.
   The service is then responsible for updating the appropriate filtered trackers when
   it hears that the objects in them might be changed.

   The service takes a controller containing a bunch of relevant functions.
]]

--- Init the FilteredTracker
--  @param controller - a controller with various functions needed for the filter. 
--
function FilteredTracker:initialize(controller)
   self._sv.controller = controller
   self:_setup_functions()
   self._sv._ids_to_keys = {}
   self._sv.tracking_data = {}
end

function FilteredTracker:restore()
   self:_setup_functions()
end

--- Sets up the functions based on these definitions
--  filter_fn - takes an entity, returns true/false if this is valid for this tracker
--  make_key_fn - takes an entity, emits the key it should have in the table
--  make_value_fn - takes an entity and existing value at that key, emits the new value (may be updated version of existing)
--  remove_value_fn - takes an entity and existing value and returns the new value sans-entity
--
function FilteredTracker:_setup_functions()
   local function_tracking_data = self._sv.controller:get_functions()
   self._remove_value_fn = function_tracking_data.remove_value_fn
end

--- Call when it's time to add an item
--
function FilteredTracker:add_item(entity)
   local controller = self._sv.controller
   local key = controller:create_key_for_entity(entity)

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
   end
end

--- Call when it's time to remove an item
--
function FilteredTracker:remove_item(entity_id)
   local key = self._sv._ids_to_keys[entity_id]
   if key then
      self._sv._ids_to_keys[entity_id] = nil

      local controller = self._sv.controller
      local tracking_data = self._sv.tracking_data[key]
      self._sv.tracking_data[key] = controller:remove_entity_from_tracking_data(entity_id, tracking_data)
   end
end

-- Returns the tracking data for the specified key.
--
function FilteredTracker:get_tracking_data_for(key)
   return self._sv.tracking_data[key]
end

return FilteredTracker
