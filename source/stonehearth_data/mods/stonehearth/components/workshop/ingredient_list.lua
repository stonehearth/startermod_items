-- This class takes some json describing the ingredients and tracks them for you.
-- The ingredient list looks like this
--
--    "ingredients": [
--        {
--          "material" : "wood resource",
--          "count" : 1
--        },
--        {
--          "material" : "fiber resource",
--          "count" : 2  
--        }
--    ]
--
local IngredientList = class()
IngredientList.__classname = 'IngredientList'

--- Start the ingredient list
-- @param entity: the entity that will be associated with all the pathfinding data
-- @param store:  an object containing a list of things that may already be collected
-- @param ingredient_data: json data containing info about the recipe
function IngredientList:__init(ingredients)
   -- clone the ingredient data so we can write to it without corrupting the
   -- copy we got from the caller
   self._missing_ingredients = {}
   for _, ingredient in ipairs(ingredients) do
      if ingredient.count > 0 then
         table.insert(self._missing_ingredients, { material = ingredient.material, count = ingredient.count })
      end
   end
end

--- Are we done?
function IngredientList:completed()
   return #self._missing_ingredients == 0
end

-- Get the next ingredient needed in the list
function IngredientList:get_next_ingredient(entity)
   if not self:completed() then
      return self._missing_ingredients[1]
   end
end

-- Check off everything currently in the container
function IngredientList:remove_contained_ingredients(entity)
   local container = entity:get_component('entity_container')
   if container then
      for id, child in container:each_child() do
         self:check_off_ingredient(child)
      end
   end
end

-- check an ingredient off the list
function IngredientList:check_off_ingredient(entity)
   for i, ingredient in ipairs(self._missing_ingredients) do
      if radiant.entities.is_material(entity, ingredient.material) then
         ingredient.count = ingredient.count - 1
         if ingredient.count <= 0 then
            table.remove(self._missing_ingredients, i)
         end
         return
      end
   end
end

return IngredientList
