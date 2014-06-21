local FilteredTracker = class()

--[[
   A service can create a filtered tracker to mantain a set of key/value pair data.
   The service is then responsible for updating the appropriate filtered trackers when
   it hears that the objects in them might be changed.

   The service takes a controller containing a bunch of relevant functions.
]]

--- Init the FilteredTracker
--  @param name - name of this tracker
--  @param player_id - player whose stuff we're tracking
--  @param fn_controller - a controller with various functions needed for the filter. 
function FilteredTracker:initialize(name, player_id, fn_controller)
   self._sv.name = name
   self._sv.player_id = player_id
   self._sv.fn_controller = fn_controller
   self:_setup_functions()
   self._sv.ids_to_keys = {}
   self._sv.data = {}
end

function FilteredTracker:restore()
   self:_setup_functions()
end

--- Sets up the functions based on these definitions
--  filter_fn - takes an entity, returns true/false if this is valid for this tracker
--  make_key_fn - takes an entity, emits the key it should have in the table
--  make_value_fn - takes an entity and existing value at that key, emits the new value (may be updated version of existing)
--  remove_value_fn - takes an entity and existing value and returns the new value sans-entity
function FilteredTracker:_setup_functions()
   local function_data = self._sv.fn_controller:get_functions()
   self._filter_fn = function_data.filter_fn
   self._make_key_fn = function_data.make_key_fn
   self._make_value_fn = function_data.make_value_fn
   self._remove_value_fn = function_data.remove_value_fn
end

--- Call when it's time to add an item
function FilteredTracker:add_item(entity)

   if self:_filter_fn(entity) then
      local key = self:_make_key_fn(entity)
      
      --Store in local id->key table
      local id = entity:get_id()
      self._sv.ids_to_keys[id] = key

      --Get existing value from key
      local existing_value = self._sv.data[key]
      local value = self._make_value_fn(self, entity, existing_value)
      self._sv.data[key] = value
   end
end

--- Call when it's time to remove an item
function FilteredTracker:remove_item(entity_id)
   local key = self._sv.ids_to_keys[entity_id]
   local existing_value = self._sv.data[key]
   local value = self._remove_value_fn(self, entity_id, existing_value)
   self._sv.data[key] = value
   self._sv.ids_to_keys[entity_id] = nil
end

function FilteredTracker:get_key(key)
   return self._sv.data[key]
end

return FilteredTracker