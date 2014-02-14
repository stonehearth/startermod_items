local CollectIngredients = class()
local IngredientList = require 'components.workshop.ingredient_list'

function CollectIngredients:run(town, args)
   local task_group = args.task_group
   local workshop = args.workshop
   local ingredients = IngredientList(args.ingredients)
   ingredients:remove_contained_ingredients(workshop)

   while not ingredients:completed() do
      local ing = ingredients:get_next_ingredient()
      local args = {
         material = ing.material,
         workshop = workshop,
         ingredient_list = ingredients,
      }

      self._task = task_group:create_task('stonehearth:collect_ingredient', args)
                          :start()
      if not self._task:wait() then
         return false
      end
      self._task = nil
   end
   return true
end

function CollectIngredients:destroy()
   if self._task then
      self._task:destroy()
      self._task = nil
   end
end

return CollectIngredients