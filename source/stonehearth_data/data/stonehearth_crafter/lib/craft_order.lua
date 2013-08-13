--[[
   Contains all the data for a CraftOrder.
   A user puts a CraftOrder in a workshop's queue so the carpenter knows what to build.

   Right now, there are 2 kinds of craft orders, mutually exclusive. They are
   defined in the "condition" parameter of the order.

   amount: X - the carpenter will build X of these items. The order will go away when
               the amount is complete.

   inventory_below: X - the carpenter will mantain X items in the town's global
                        inventory. This condition remains on the queue, and is
                        evaluated on each pass through the queue, but is
                        skipped whenever there are X or more items in the town.

   Theoretically, future condtions do not need to be mutually exclusive, but these two are.

   The format for an order. Note, amount and inventory_below are mutually exclusive
   {
      "id":"some number uniquely referencing this object
      "workshop: ":workshop this order belongs to
      "recipe":"/module_name/craftable/object_recipe.json",
      "enabled":"true/false",
      "condition": {
         "amount":"an integer",
         "inventory_below":"integer to mantain at"
      }
      "status": {
         "amount_made":"num made so far, counts against amount"
      }
   }
]]

local CraftOrder = class()

--(belongs to CraftOrder) variable to assign order IDs
local craft_order_id = 1

--[[
Create a new CraftOrder
   recipe:     The name of the thing to build in mod:// format
   enabled:    Whether or not the user currently wants the crafter to
               evaluate the order
   condition:  The parameters that must be met for the carpenter to make
               the order.
]]

function CraftOrder:__init(recipe, enabled, condition, workshop)
   self._id = craft_order_id
   craft_order_id = craft_order_id + 1

   self._recipe = recipe
   self._enabled = enabled
   self._condition = condition
   self._workshop = workshop
   
   local faction = self._workshop:get_entity():add_component('unit_info'):get_faction()
   assert(faction and (#faction > 0), "workshop has no faction.")
   
   self._status = {}

   -- look out into the world of all the ingredients
   self._ingredients = {}
   for offset, ingredient_data in radiant.resources.pairs(self._recipe.ingredients) do
      local filter = function(item_entity)
         return self:_can_use_ingredient(item_entity, ingredient_data)
      end
      local ingredient = {
         item = nil,
         path = nil,
         filter = filter
      }
      table.insert(self._ingredients, ingredient)
   end   
end

function CraftOrder:__tojson()
   local json = {
      recipe = self._recipe,
      condition = self._condition,
      enabled = self._enabled
   }
   return radiant.json.encode(json)
end

function CraftOrder:_can_use_ingredient(item_entity, ingredient_data)
   -- make sure it's an item...
   local item = item_entity:get_component('item')
   if not item then
      return false
   end

   if not item:get_material() == ingredient_data.material then
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

function CraftOrder:search_for_ingredients()
   local bench_items = self._workshop:get_items_on_bench()
   local workshop_entity = self._workshop:get_entity()

   if self._found_all_ingredients then
      return
   end
   if self._search_running then
      return   
   end
   
   for i, ingredient in ipairs(self._ingredients) do    
      if not ingredient.item then
         -- is the item on the bench?  if so, claim it and remove it from
         -- the bench list..
         for _, item in pairs(bench_items) do
            if ingredient.filter(item) then
               ingredient = item
               bench_items[_] = nil
               break
            end
         end
         if not ingredient.item then
            -- not on the bench?  look around and see if we can find an item
            local solved = function(path)
               local item = path:get_destination()
               ingredient.path = path
               ingredient.item = item
               
               self._pathfinder:stop()
               self._pathfinder = nil
               self._search_running = false
               self:search_for_ingredients()
            end
            self._pathfinder = radiant.pathfinder.find_path_to_closest_entity(
                                 'workshop searching for items',
                                 workshop_entity,
                                 solved,
                                 ingredient.filter)
            self._search_running = true
            return
         end
      end
   end
   -- we've got everything! woot!
   self._found_all_ingredients = true
end

-- Getters and Setters

function CraftOrder:get_id()
   return self._id
end

function CraftOrder:get_recipe()
   return self._recipe
end

function CraftOrder:get_enabled()
   return self._enabled
end

function CraftOrder:toggle_enabled()
   self._enabled = not self._enabled
end

function CraftOrder:get_condition()
   return self._condition
end

--[[
   Used to determine if we should proceed with executing the order.
   If this order has a condition which are unsatisfied, (ie, less than x amount
   was built, or less than x inventory exists in the world) return true.
   If this order's conditions are met, and we don't need to execute this
   order, return false.
   returns: true if conditions are not yet met, false if conditions are met
]]
function CraftOrder:should_execute_order()
   if self._condition.amount then
      return true
   end
   if self._condition.inventory_below then
      --TODO: access the inventory
      --TODO: ask if the inventory has self.condition.inventory_below of recipe object
      --TODO: if not, return true
      return true
      --otherwise, return false
   end
end

--[[
   Check if the ingredients for an order are available in the
   inventory and workbench. If so, reserve them and return true.
   Otherwise, return false.
   TODO: actually integrate with the inventory, clean up code
   TODO: test what happens if some ingredients are already on the bench.

   returns: true if ingredients are avaialble, false otherwise
]]
function CraftOrder:get_all_ingredients()
   -- xxx: This implementation can be improved...there may be solutions that
   -- it does not find because of overlapping tags (e.g. a recipe that requires
   -- "magic wood" and "magic" blocks, where the "magic wood" block is on the
   -- bench and there are no "magic" items in the world)
   
   if self._found_all_ingredients then
      -- xxx: do more validation?
      return self._ingredients
   end
   --[[
   
   local ingredients = {}
   local bench_items = self._workshop:get_items_on_bench()

   for offset, ingredient in radiant.resources.pairs(self._recipe.ingredients) do
      local i = offset + 1

      -- is the item on the bench?  if so, claim it and remove it from
      -- the bench list..
      for _, item in pairs(bench_items) do
         if self._item_is_suitable_for_ingredient(item, ingredient) then
            ingredients[i] = {
               item = item
            }  
            bench_items[_] = nil
         end
      end
      -- wasn't on the bench?  bummer.  see if we know how to get one in the
      -- world.  if so, remember both the item and the path so we can get to
      -- it (eventually)
      if not ingredients[i] then
         local path_info = self._ingredient_paths[offset]
         if path_info and path_info.item then         
            ingredients[i] = {
               item = path_info.item,
               path = path_info.path,
            }
         end
      end
      -- not on the bench and not in the world.  we're really really really
      -- out of luck.  return nil.
      if not ingredients[i] then
         return nil
      end
   end

   -- return all the ingredients we need to build the recipe
   return ingredients
   ]]
   --[[
   for i, ingredient in radiant.resources.pairs(self._recipe.ingredients) do
      --TODO check if inventory + workbench has at least amount of ingredient
      --if not return false
   end
   --if all the ingredients are present, reserve them and their
   --associated paths in the inventory, using this order_id
   local ingredient_obj = self._recipe.ingredients
   for ingredient, amount in radiant.resources.pairs(ingredient_obj) do
      --check if the ingredients are already on the workbench
      local num_available = self._workshop:num_ingredients_on_bench(ingredient)
      local num_needed = 0
      if num_available and num_available < amount then
         num_needed = amount - num_available
      elseif not num_available then
         num_needed = amount
      end
      --if there are outstanding ingredients, find them in the world
      while num_needed > 0 do
         --look up the remaining ingredients
         --TODO reserve the paths to ingredients with the order_id or entity id
         --For each ingredient, we'll need its entity so we can calculate paths.
         num_needed = num_needed - 1
      end
      --NOTE we do not lock the inventory, since really, it's single threaded?
   end
   return true
   ]]
end

--[[
   Used to determine if an order is complete. Inventory_below orders
   are never complete, as the crafter is always monitoring if
   they should be making more.
   returns: true if the order is complete and can be removed from
             the list, false otherwise.
]]
function CraftOrder:check_complete()
   if self._condition.amount then
      if not self._status.amount_made then
         self._status.amount_made = 1
      else
         self._status.amount_made = self._status.amount_made + 1
      end
      return self._status.amount_made == self._condition.amount

   elseif self._condition.inventory_below then
      return false
   end

end

return CraftOrder