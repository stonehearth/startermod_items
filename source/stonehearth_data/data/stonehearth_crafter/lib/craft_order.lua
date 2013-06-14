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
      "recipe":"mod://module_name/craftable/object_recipe.txt",
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
   self._status = {}
   --Note: I suppose we could make a function that added status based on conditions
end

-- Getters and Setters

function CraftOrder:get_id()
   return self._id
end

function CraftOrder:get_recipe()
   return self._recipe
end

function CraftOrder:set_recipe(recipe)
   self._recipe = recipe
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
   Whether or not an order can be executed right now is a factor
   of whether its ingredients are in the world and whether its
   conditions are not yet satisfied.
   returns: true if conditions are not yet met AND ingredients are available
]]
function CraftOrder:can_execute_order()
   return self:are_conditions_unsatisfied() and self:are_ingredients_available()
end

--[[
   If this order has a condition which is not yet met, (ie, less than x amount
   was built, or less than x inventory exists in the world) return true.
   If this order's conditions are met, and we don't need to execute this
   order, return false.
]]
function CraftOrder:are_conditions_unsatisfied()
   if self._condition.amount then
      return true
   elseif self._condition.inventory_below then
      --TODO: access the inventory
      --TODO: ask if the inventory has self.condition.inventory_below of recipe object
      --TODO: if not, return true
      return true
      --otherwise, return false
   end
end

--[[
   Check if the ingredients for an order are available
   If so, reserve them and return true. Otherwise, return
   false.
]]
function CraftOrder:are_ingredients_available()
   for ingredient, amount in radiant.resources.pairs(self._recipe.ingredients) do
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
      while num_needed > 0 do
         --look up the remaining ingredients
         --TODO reserve the paths to ingredients with the order_id or entity id
         --For each ingredient, we'll need the path to the ingredient, the path to the workbench, and a ref to the ingredient itself
         num_needed = num_needed - 1
      end
      --NOTE we do not lock, since really, it's single threaded?
   end
   return true
end

--[[
   Used to determine if an order is complete. Inventory_below orders are never
   complete, as the crafter is always monitoring if they should be making more.
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