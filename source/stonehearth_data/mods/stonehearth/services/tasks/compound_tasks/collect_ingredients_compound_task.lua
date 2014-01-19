local CollectIngredients = class()
local IngredientList = require 'components.workshop.ingredient_list'

function CollectIngredients:start(thread, args)
   self._workshop = args.workshop
   self._ingredients = IngredientList(args.ingredients)
end

function CollectIngredients:run(thread, args)
   self._ingredients:remove_contained_ingredients(self._workshop)

   while not self._ingredients:completed() do
      local ing = self._ingredients:get_next_ingredient()
      local args = {
         material = ing.material,
         workshop = self._workshop,
         ingredient_list = self._ingredients,
      }
      thread:run_task('stonehearth:collect_ingredient', args)
   end
end

stonehearth.tasks:register_compound_task('stonehearth:tasks:collect_ingredients', CollectIngredients)
