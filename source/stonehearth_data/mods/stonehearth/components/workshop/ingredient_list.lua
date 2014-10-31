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
function IngredientList:__init(workshop, ingredients)
   -- clone the ingredient data so we can write to it without corrupting the
   -- copy we got from the caller
   self._workshop = workshop
   self._remaining_ingredients = {}
   for _, ingredient in ipairs(ingredients) do
      for i=1, ingredient.count do
         table.insert(self._remaining_ingredients, ingredient)
      end
   end
end

--- Are we done?
function IngredientList:completed(workshop)
   return #self._remaining_ingredients == 0
end

-- Get the next ingredient needed in the list
function IngredientList:get_next_ingredient(entity)
   return self._remaining_ingredients[1]
end

-- check an ingredient off the list
function IngredientList:check_off_ingredient(new_ingredient)
   for i, ingredient in ipairs(self._remaining_ingredients) do
      if ingredient.material == new_ingredient.material or ingredient.uri == new_ingredient.uri then
         table.remove(self._remaining_ingredients, i)
         return
      end
   end
end

--[[
function IngredientList:_ingredient_on_workshop(ingredient)
   local container = self._workshop:get_component('entity_container')
   if container then
      for id, child in container:each_child() do
         -- is the entitty of a material that matches the required ingredient?
         if ingredient.material ~= nil then
            if radiant.entities.is_material(child, ingredient.material) then
               return true
            end
         else
            assert(ingredient.uri ~= nil, 'ingredient list entry specifies neither a material nor a uri')
            -- does the entity's alias match the one specified in the ingredient?
            local uri = child:get_uri()
            if (uri == ingredient.uri) then
               return true
            end
         end
      end
   end

   return false
end
]]

return IngredientList
