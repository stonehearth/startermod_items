local UsableItemTracker = class()

--[[
   Track everything in public storage plus the stuff on the ground, which is stuff whose mob's parent is 1
   Track by uri and by material (yes, that means material combinatorics, sigh)
   So if a thing is "food resource fruit" that means its keys are: 
      food
      resource
      fruit
      food resource
      food fruit
      resource fruit
   The goal is to use this tracker for crafting/building UIs, to show if we have enough
   to accomplish a task. Also, for crafters, so they can skip tasks they know they won't have
   resources to accomplish.

   Note, the tags are ordered by table.sort, if you want to look them up by resource combo, alphabetize before lookup!
]]

function UsableItemTracker:initialize()
end

function UsableItemTracker:restore()
end

function UsableItemTracker:create_keys_for_entity(entity, storage)
   assert(entity:is_valid(), 'entity is not valid.')
   local uri = entity:get_uri()

   local in_public_storage
   local on_ground = self:_is_item_on_ground(entity)

   if storage then
      local storage_component = storage:get_component('stonehearth:storage') 
      if storage_component and storage_component:is_public() then
         in_public_storage = true
      end
   end

   if in_public_storage or on_ground then
      --calculate all the material keys
      local keys = self:_calculate_material_keys(entity)

      --append the uri to the material keys
      keys = table.insert(keys, uri)

      --return the combined array
      return keys
   end
end

--Returns true if the item is on the ground and available for grabbing
function UsableItemTracker:_is_item_on_ground(entity)
   local mob = entity:add_component('mob')
   local parent = mob:get_parent()
   return parent and parent:get_id() == 1 
end

-- Get all the material keys for this item
function UsableItemTracker:_calculate_material_keys(entity)
   local material_component = entity:get_component('stonehearth:material')
   local key_array = {}
   if material_component then
      key_array = {}
      local tag_string = material_component:get_tags()
      if tag_string then
         --split, sort, 
         self:_calculate_keys(self:_sort_string(tag_string), key_array)
      end
   end
   return key_array
end

--Given a string of items, turn into table, sort table, turn back into string
function UsableItemTracker:_sort_string(tag_string)
   --Now, for each material in the string, remove it and send the remaining sub-string on
   local tag_array = radiant.util.split_string(tag_string) 
   table.sort(tag_array)
   return table.concat(tag_array, ' ')
end

function UsableItemTracker:_has_key(key_array, target_key)
   for i, key in ipairs(key_array) do 
      if target_key == key then
         return true
      end
   end
   return false
end

-- Recursively calculate all permutations of material keys
-- @param material_string: string of materials, each separated by a space
function UsableItemTracker:_calculate_keys(material_string, key_array)
   --add the string to the key array

   if not self:_has_key(key_array, material_string) then
      table.insert(key_array, material_string)
   end

   --Now, for each material in the string, remove it and send the remaining sub-string on
   local tag_array = radiant.util.split_string(material_string) 

   --If there's only one tag, then just return early 
   if #tag_array == 1 then
      return
   end

   --If there's more than one tag, then remove one, generate a new string, and call fn recursively
   for i, tag in ipairs(tag_array) do 
      --make a copy of the array without the tag
      local tag_array_copy = {}
      for j, tag in ipairs(tag_array) do 
         if j ~= i then
            table.insert(tag_array_copy, tag)
         end
      end
      local new_string = table.concat(tag_array_copy, ' ')
      self:_calculate_keys(self:_sort_string(new_string), key_array)
   end
end

function UsableItemTracker:add_entity_to_tracking_data(entity, tracking_data)
   if not tracking_data then
      --create the tracking data for this type of object
      local unit_info = entity:add_component('unit_info')
      tracking_data = {
         uri = entity:get_uri(),
         count = 0, 
         items = {},
         first_item = entity
      }
   end

   local id = entity:get_id()
   if not tracking_data.items[id] then
      tracking_data.count = tracking_data.count + 1
      tracking_data.items[id] = entity
   end
   return tracking_data   
end

function UsableItemTracker:remove_entity_from_tracking_data(entity_id, tracking_data)
   if not tracking_data then
      return nil
   end

   if tracking_data.items[entity_id] then
      tracking_data.items[entity_id] = nil
      tracking_data.count = tracking_data.count - 1

      if tracking_data.count <= 0 then
         tracking_data.first_item = nil
         return nil
      end

      local id, next_item = next(tracking_data.items)
      tracking_data.first_item = next_item
   end
   return tracking_data
end

return UsableItemTracker