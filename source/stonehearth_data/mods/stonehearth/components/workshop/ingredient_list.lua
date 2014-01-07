--This class takes some json describing the ingredients, stores it in a table,
--and sets up pathfinders for each item.  
-- "ingredients": [
--     {
--       "material" : "wood resource",
--       "count" : 1
--     },
--     {
--       "material" : "fiber resource",
--       "count" : 2  
--     }
-- ]
local IngredientList = class()

--- Start the ingredient list
-- @param entity: the entity that will be associated with all the pathfinding data
-- @param store:  an object containing a list of things that may already be collected
-- @param ingredient_data: json data containing info about the recipe
function IngredientList:__init(entity, existing_store, ingredient_data)
   self._entity = entity
   self._ingredient_data = ingredient_data
   self._ingredients = {}
   self._existing_store = existing_store

   self._found_all_ingredients = false
   self._search_running = false

   self:_prep_ingredient_data()
end

--- Creates a new set of ingredients based on the data
function IngredientList:_prep_ingredient_data()
   for offset, ingredient_data in ipairs(self._ingredient_data) do
      local filter = function(item_entity)
         return self:_can_use_ingredient(item_entity, ingredient_data)
      end
      for i=1, ingredient_data.count do
         local ingredient = {
            item = nil,
            path = nil,
            filter = filter
         }
         table.insert(self._ingredients, ingredient)
      end
   end
end

--- Checks an item against the ingredient list. 
--  @returns true if we can use it, false if we can't
function IngredientList:_can_use_ingredient(item_entity, ingredient_data)
   -- make sure it's an item of the right material
   local material = item_entity:get_component('stonehearth:material')
   if not material or not material:is(ingredient_data.material) then
      return false
   end

   -- make sure we're not using it for something else...
   for _, ingredient in ipairs(self._ingredients) do
      if ingredient.item and item_entity:get_id() == ingredient.item:get_id() then
         return false
      end
   end

   return true
end

-- Look into the world for ingredients and claim them. 
function IngredientList:search_for_ingredients()
   if self._found_all_ingredients then 
      return
   end
   if self._search_running then
      return
   end

   for i, ingredient in ipairs(self._ingredients) do
      if not ingredient.item or not ingredient.item:is_valid() then
         local found = false
         for _, item in pairs(self._existing_store) do 
            --is the item in the existing store? if so, mark found
            if ingredient.filter(item) then
               ingredient.item = item
               self._existing_store[_] = nil
               found = true
               break
            end
         end
         if not found then
            --Not in store? look around to find
            local solved = function(path)
               local item = path:get_destination()
               ingredient.path = path
               ingredient.item = item
               if self._pathfinder then
                  self._pathfinder:stop()
               end
               self._pathfinder = nil
               self._search_running = false
               self:search_for_ingredients()
            end
            self._pathfinder = radiant.pathfinder.create_path_finder(self._entity, 'workshop searching for items')
                                  :set_solved_cb(solved)
                                  :set_filter_fn(ingredient.filter)
                                  :find_closest_dst()
            self._search_running = true
            return
         end
      end
   end
   -- we've got everything! woot!
   self._found_all_ingredients = true
end

--- If we've found the ingredients, return them.
--  If we have not found the ingredients, return nothing. 
--  xxx: This implementation can be improved...there may be solutions that
--  it does not find because of overlapping tags (e.g. a recipe that requires
--  "magic wood" and "magic" blocks, where the "magic wood" block is on the
--  bench and there are no "magic" items in the world)
function IngredientList:get_all_ingredients()
   if self._found_all_ingredients then
      return self._ingredients
   end
end

return IngredientList