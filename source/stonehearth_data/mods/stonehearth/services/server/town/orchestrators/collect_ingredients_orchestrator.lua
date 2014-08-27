local CollectIngredients = class()
local IngredientList = require 'components.workshop.ingredient_list'

function CollectIngredients:run(town, args)
   local task_group = args.task_group
   local workshop = args.workshop
   local ingredients = IngredientList(args.ingredients)
   self._craft_order_list = args.order_list
   self._order = args.order

   if self._craft_order_list and self._order then
      self._order_list_listener = radiant.events.listen(self._craft_order_list, 'stonehearth:order_list_changed', self, self._on_order_list_changed)
   end

   ingredients:remove_contained_ingredients(workshop)

   while not ingredients:completed() do
      local ing = ingredients:get_next_ingredient()
      local args = {
         material = ing.material,
         workshop = workshop,
         ingredient_list = ingredients,
      }

      self._task = task_group:create_task('stonehearth:collect_ingredient', args)
                          :set_priority(stonehearth.constants.priorities.top.CRAFT)
                          :once()
                          :start()
      if not self._task:wait() then
         return false
      end
      self._task = nil
   end
   return true
end

-- If OUR order is deleted kill the task
-- TODO: (Or is no longer the top order?)
-- TODO: It would be even better if we could specify that the pathfiner should stop
-- if it can't find anything!
function CollectIngredients:_on_order_list_changed()
   local order_index = self._craft_order_list:find_index_of(self._order:get_id())
   if not order_index then
      self:destroy()
   end
end

function CollectIngredients:destroy()
   if self._task then
      self._task:destroy()
      self._task = nil
   end
   
   if self._craft_order_list then
      self._order_list_listener:destroy()
      self._order_list_listener = nil
   end
end

return CollectIngredients