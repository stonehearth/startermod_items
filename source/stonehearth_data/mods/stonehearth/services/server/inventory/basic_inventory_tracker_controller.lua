local BasicInventoryTrackerController = class()

--[[
   Holds the functions used by the basic inventory tracker
   Packaged as a controller so the inventory tracker has easy access to them on creation
]]

function BasicInventoryTrackerController:initialize()
end

function BasicInventoryTrackerController:restore()
end

-- Functions to get the functions --
function BasicInventoryTrackerController:get_functions()
   local filtered_tracker_fn_bundle = {
      filter_fn = self.filter_fn,
      make_key_fn = self.make_key_fn,
      make_value_fn = self.make_value_fn,
      remove_value_fn = self.remove_value_fn
   }
   return filtered_tracker_fn_bundle
end

-- Here are the actual functions required for the tracker ---

function BasicInventoryTrackerController:filter_fn(entity)
   assert(entity:is_valid(), 'entity is not valid. Make sure to call before destroy')
   return true
end

function BasicInventoryTrackerController:make_key_fn(entity)
   return entity:get_uri()
end

--The value is an object with
-- a. a map of the entities of this type, by their ID. 
-- b. a count of the number of entities of this type
function BasicInventoryTrackerController:make_value_fn(entity, existing_value)
   if not existing_value then
      --We're the first object of this type
      existing_value = {
         count = 0, 
         items = {}
      }
   end
   local prev_entry = existing_value.items[entity:get_id()]
   if not prev_entry then
      existing_value.count = existing_value.count + 1
   end   
   existing_value.items[entity:get_id()] = entity 
   return existing_value
end

function BasicInventoryTrackerController:remove_value_fn(entity, existing_value)
   --If for some reason, there's no existing value for this key, just return
   if not existing_value then
      return nil
   end

   local id = entity:get_id()
   if existing_value.items[id] then
      existing_value.count = existing_value.count - 1
      assert(existing_value.count >= 0)
   end
   existing_value.items[id] = nil
   return existing_value
end

return BasicInventoryTrackerController