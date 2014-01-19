local CraftItem = class()

function CraftItem:start(thread, args)
   self._thread = thread
   self._workshop = args.workshop
   self._craft_order_list = args.craft_order_list

   radiant.events.listen(self._craft_order_list, 'order_list_changed', self, self._on_order_list_changed)
   self:_on_order_list_changed(self._craft_order_list, not self._craft_order_list:get_next_order())
end

function CraftItem:run(thread, args)
   while true do
      local order =  self:_get_next_order()      

      order:set_crafting_status(true)
      self:_collect_ingredients(order)
      self:_process_order(order)
      order:set_crafting_status(false)
      
      if order:is_complete() then
         self._craft_order_list:remove_order(order)
      end

      self:run_task('stonhearth:clear_workshop_bench', {
         workshop = self._workshop,
      })
   end
end

function CraftItem:stop(thread, args)
   radiant.events.unlisten(self._craft_order_list, 'order_list_changed', self, self._on_order_list_changed)
end

function CraftItem:_collect_ingredients(order)
   local recipe = order:get_recipe()
   
   thread:run_task('stonehearth:tasks:collect_ingredients', {
      workshop = self._workshop,
      ingredients = recipe.ingredients,
   })
end

function CraftItem:_process_order(order)
   local times = order:get_work_units()
   local effect = order:get_work_effect()
   local recipe = order:get_recipe()

   thread:run_task('stonehearth:work_at_workshop', {
      workshop = self._workshop,
      effect = effect,
      times = times,
   })

   self:_destroy_items_on_bench()
   self:_add_outputs_to_bench(recipe)
end

function CraftItem:_get_next_order()
   while true do
      local order 
      if not self._craft_order_list:is_paused() then
         order = self._craft_order_list:get_next_order()
      end
      if order ~= nil then
         return order
      end
      self._waiting_for_order = true
      self._thread:suspend_thread()
      self._waiting_for_order = false
   end
end

function CraftItem:_add_outputs_to_bench(recipe)

   -- create all the recipe products
   for i, product in ipairs(recipe.produces) do
      local item = radiant.entities.create_entity(product.item)
      item:add_component('mob'):set_location_grid_aligned(Point3(0, 1, 0))

      self._workshop:add_component('entity_container'):add_child(item)
      stonehearth.analytics:send_design_event('game:craft', self._workshop, item)
   end
end

function CraftItem:_destroy_items_on_bench()
   for id, item in self._workshop:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end
end


return CraftItem
