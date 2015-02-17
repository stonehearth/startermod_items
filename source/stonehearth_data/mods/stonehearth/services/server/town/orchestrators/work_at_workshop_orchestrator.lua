local Point3 = _radiant.csg.Point3
local CollectIngredients = require 'services.server.town.orchestrators.collect_ingredients_orchestrator'
local ClearWorkshop = require 'services.server.town.orchestrators.clear_workshop_orchestrator'
local rng = _radiant.csg.get_default_rng()

local WorkAtWorkshop = class()

function WorkAtWorkshop:run(town, args)
   self._town = town
   self._thread = stonehearth.threads:get_current_thread()
   self._workshop = args.workshop
   self._crafter = args.crafter
   self._craft_order_list = args.craft_order_list

   self._task_group = town:create_task_group('stonehearth:crafting', {})
                                                  :add_worker(self._crafter)

   self._inventory = stonehearth.inventory:get_inventory(town:get_player_id())
   self._order_changed_listener = radiant.events.listen(self._craft_order_list, 'stonehearth:order_list_changed', self, self._on_order_list_changed)
   self:_on_order_list_changed(self._craft_order_list, not self._craft_order_list:get_next_order())

   --Listen on this to re-check mantain whenever an item is removed from the stockpile
   self._item_removed_listener = radiant.events.listen(self._inventory, 'stonehearth:inventory:item_removed', self, self._on_order_list_changed)

   while true do
      local order = self:_get_next_order()

      order:set_crafting_status(true)

      --Rewrite this loop so that this can fail when the order in question is deleted
      --EC: add a new task if that new task is added to the top of the queue
      local collection_success = self:_collect_ingredients(order) 
      if collection_success then
         self:_process_order(order)
         
         if order:is_complete() then
            self._craft_order_list:remove_order(order)
         end
         order:set_crafting_status(false)
      end
      self._town:run_orchestrator(ClearWorkshop, {
         crafter = self._crafter,
         task_group = self._task_group,
         workshop = self._workshop
      })
   end
end

function WorkAtWorkshop:stop(args)
   self:destroy()
end

function WorkAtWorkshop:destroy()
   if self._order_changed_listener then
      self._order_changed_listener:destroy()
      self._order_changed_listener = nil
   end

   if self._item_removed_listener then
      self._item_removed_listener:destroy()
      self._item_removed_listener = nil
   end
end

function WorkAtWorkshop:_collect_ingredients(order)
   local recipe = order:get_recipe()

   local result = self._town:run_orchestrator(CollectIngredients, {
      task_group = self._task_group,
      workshop = self._workshop,
      ingredients = recipe.ingredients, 
      order_list = self._craft_order_list,
      order = order
   })
   
   return result
end

--- Run to the workshop and work. If the workshop has moved, run there. 
function WorkAtWorkshop:_process_order(order)
   local recipe = order:get_recipe()

   local crafter = self._workshop:get_component('stonehearth:workshop')
                                 :get_crafter()
   local effect = crafter:get_component('stonehearth:crafter')
                         :get_work_effect()
   local args = {
      workshop = self._workshop,
      times = 1,
      effect = effect, 
      item_name = recipe.recipe_name
   }
   local task = self._task_group:create_task('stonehearth:work_at_workshop', args)
                                     :set_priority(stonehearth.constants.priorities.crafting.DEFAULT)
                                     :times(recipe.work_units)
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

--TODO: add trigger for item deletion
function WorkAtWorkshop:_on_order_list_changed()
   if self._waiting_for_order then
      self._thread:resume()
   end   
end

function WorkAtWorkshop:_add_outputs_to_bench(recipe)
   -- create all the recipe products
   for i, product in ipairs(recipe.produces) do
      --If we're a certain level of crafter, we can make fine versions of objects
      local item = radiant.entities.create_entity(self:_determine_output(product), { owner = self._crafter })
      local entity_forms = item:get_component('stonehearth:entity_forms')
      if entity_forms then
         local iconic_entity = entity_forms:get_iconic_entity()
         if iconic_entity then
            item = iconic_entity
         end
      end
      item:add_component('mob'):set_location_grid_aligned(Point3(0, 1, 0))
      radiant.entities.set_player_id(item, self._crafter)

      self._workshop:add_component('entity_container'):add_child(item)

      --Tell the crafter to update its recipe requirements
      --TODO: consider adding this back if we ever implement make X to get Y recipes
      --local crafter_component = self._crafter:get_component('stonehearth:crafter')
      --crafter_component:update_locked_recipes(item:get_uri())

      --send event that the carpenter has finished an item
      local crafting_data = {
         recipe_data = recipe, 
         product = item
      }
      radiant.events.trigger_async(self._crafter, 'stonehearth:crafter:craft_item', crafting_data)

   end
end

-- Does the crafter have a chance of making a fine object? 
-- Is there a chance that this object might be fine? If so, make one!
-- Crafter's chance of getting a fine object goes from 0 to some number on level up.
function WorkAtWorkshop:_determine_output(product)
   local item_uri = product.item
   local target_num = rng:get_int(1, 100)
   local crafter_component = self._crafter:get_component('stonehearth:crafter')
   if product.fine and target_num <= crafter_component:get_fine_percentage() then
      item_uri = product.fine
   end
   return item_uri
end

function WorkAtWorkshop:_destroy_items_on_bench()
   for id, item in self._workshop:add_component('entity_container'):each_child() do
      radiant.entities.destroy_entity(item)
   end
end

return WorkAtWorkshop
