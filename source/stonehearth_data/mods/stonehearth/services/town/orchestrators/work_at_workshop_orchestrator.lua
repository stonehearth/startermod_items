local Point3 = _radiant.csg.Point3
local CollectIngredients = require 'services.town.orchestrators.collect_ingredients_orchestrator'
local ClearWorkshop = require 'services.town.orchestrators.clear_workshop_orchestrator'

local WorkAtWorkshop = class()

function WorkAtWorkshop:run(town, args)
   self._town = town
   self._thread = stonehearth.threads:get_current_thread()
   self._workshop = args.workshop
   self._crafter = args.crafter
   self._craft_order_list = args.craft_order_list

   self._task_group = town:create_task_group('stonehearth:top', {})
                                                  :set_priority(stonehearth.constants.priorities.top.CRAFT)
                                                  :add_worker(self._crafter)

   local faction = radiant.entities.get_faction(self._crafter)
   self._inventory = stonehearth.inventory:get_inventory(faction)

   radiant.events.listen(self._craft_order_list, 'order_list_changed', self, self._on_order_list_changed)
   self:_on_order_list_changed(self._craft_order_list, not self._craft_order_list:get_next_order())

   --Listen on this to re-check mantain whenever an item is removed from the stockpile
   radiant.events.listen(self._inventory, 'stonehearth:item_removed', self, self._on_order_list_changed)

   while true do
      local order = self:_get_next_order()

      order:set_crafting_status(true)
      self:_collect_ingredients(order)
      self:_process_order(order)
      order:set_crafting_status(false)
      
      if order:is_complete() then
         self._craft_order_list:remove_order(order)
      end

      self._town:run_orchestrator(ClearWorkshop, {
         task_group = self._task_group,
         workshop = self._workshop
      })
   end
end

function WorkAtWorkshop:stop(args)
   radiant.events.unlisten(self._craft_order_list, 'order_list_changed', self, self._on_order_list_changed)
   radiant.events.unlisten(self._inventory, 'stonehearth:item_removed', self, self._on_order_list_changed)
end

function WorkAtWorkshop:_collect_ingredients(order)
   local recipe = order:get_recipe()

   self._town:run_orchestrator(CollectIngredients, {
      task_group = self._task_group,
      workshop = self._workshop,
      ingredients = recipe.ingredients
   })
end

function WorkAtWorkshop:_process_order(order)
   local recipe = order:get_recipe()

   local args = {
      workshop = self._workshop,
      times = recipe.work_units
   }
   local task = self._task_group:create_task('stonehearth:work_at_workshop', args)
                                     :once()
                                     :start()
   if not task:wait() then
      return false
   end
   
   self:_destroy_items_on_bench()
   self:_add_outputs_to_bench(recipe)
   order:on_item_created()
   return true
end

function WorkAtWorkshop:_get_next_order()
   while true do
      local order 
      if not self._craft_order_list:is_paused() then
         order = self._craft_order_list:get_next_order()
      end
      if order ~= nil then
         return order
      end
      self._waiting_for_order = true
      self._thread:suspend()
      self._waiting_for_order = false
   end
end

function WorkAtWorkshop:_on_order_list_changed()
   if self._waiting_for_order then
      self._thread:resume()
   end   
end

function WorkAtWorkshop:_add_outputs_to_bench(recipe)
   -- create all the recipe products
   for i, product in ipairs(recipe.produces) do
      local item = radiant.entities.create_entity(product.item)
      item:add_component('mob'):set_location_grid_aligned(Point3(0, 1, 0))

      self._workshop:add_component('entity_container'):add_child(item)
      stonehearth.analytics:send_design_event('game:craft', self._workshop, item)
   end
end

function WorkAtWorkshop:_destroy_items_on_bench()
   for id, item in self._workshop:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end
end

return WorkAtWorkshop
