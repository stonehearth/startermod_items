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
   self:_alter_data(entity, self._make_value_fn)
end

--- Call when it's time to remove an item
function FilteredTracker:remove_item(entity)
   self:_alter_data(entity, self._remove_value_fn)
end

function FilteredTracker:get_key(key)
   return self._sv.data[key]
end

--- Alter the data based on the type of operation
--  If this is an entity that passes the filter, calculate the key
--  Then, based on the existing value for that key, calculate a new/updated entry.
--  This is allows our values to be things like arrays of arrays of the entity
function FilteredTracker:_alter_data(entity, manipulation_fn)
   if self:_filter_fn(entity) then
      local key = self:_make_key_fn(entity)
      local existing_value = self._sv.data[key]
      local value = manipulation_fn(self, entity, existing_value)
      self._sv.data[key] = value
   end
end

return FilteredTracker