local CollectIngredients = class()
local IngredientList = require 'components.workshop.ingredient_list'

function CollectIngredients:__init(scheduler, ingredients_json, workshop)
   self._workshop = workshop
   self._scheduler = scheduler
   self._ingredients = IngredientList(ingredients_json)
   self._ingredients:remove_contained_ingredients(workshop)
end

function CollectIngredients:start()
   self:_gather_next_ingredient()
end

function CollectIngredients:_gather_next_ingredient()
   -- gather the ingredients one by one
   if self._ingredients:completed() then
      -- trigger completed...
   else
      local ing = self._ingredients:get_next_ingredient()
      local name = string.format('get %s for workshop', ing.material)
      local args = {
         material = ing.material,
         workshop = self._workshop,
         ingredient_list = self._ingredients,
      }
      local task = self._scheduler:create_task('stonehearth:collect_ingredient', args)
                                   :set_name(name)
                                   :once()
                                   :start()
      radiant.events.listen(task, 'completed', function()
            self:_gather_next_ingredient()
            radiant.events.trigger(self, 'completed')
            return radiant.events.UNLISTEN
         end)
   end
end

return CollectIngredients
