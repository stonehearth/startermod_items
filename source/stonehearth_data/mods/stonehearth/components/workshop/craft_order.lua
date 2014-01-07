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

local object_tracker = require 'services.object_tracker.object_tracker_service'
local IngredientList = require 'components.workshop.ingredient_list'

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
   ingredients:TODO: the ingredients chosen by the user for the object
]]

function CraftOrder:__init(recipe, enabled, condition, workshop)
   self._id = craft_order_id
   craft_order_id = craft_order_id + 1

   self._recipe = recipe
   self._enabled = enabled
   self._condition = condition
   if self._condition.inventory_below then
      self._condition.inventory_below = tonumber(self._condition.inventory_below)
   end

   self._workshop = workshop

   --TODO: call a function to figure out the queue art from the ingredients
   self._portrait = recipe.portrait

   self._faction = self._workshop:get_entity():add_component('unit_info'):get_faction()
   assert(self._faction and (#self._faction > 0), "workshop has no faction.")

   self._status = {is_crafting = false}

   self._ingredient_list_object = IngredientList(
      self._workshop:get_entity(), 
      self._workshop:get_items_on_bench(), 
      self._recipe.ingredients)
end

function CraftOrder:__tojson()
   local json = {
      id = self._id,
      recipe = self._recipe,
      condition = self._condition,
      enabled = self._enabled,
      portrait = self._portrait,
      status = self._status
   }
   if self._condition.amount then
      self._condition.amount = tonumber(self._condition.amount)
      json.remaining = self._condition.amount
      if self._status.amount_made then
         json.remaining = self._condition.amount - self._status.amount_made
      end
   end
   return radiant.json.encode(json)
end

--[[
   Destructor??
]]
function CraftOrder:destroy()
   --Clean up any other things, for example:
   self._pathfinder = nil
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

function CraftOrder:set_crafting_status(status)
   self._status.is_crafting = status;
end

--- Look into the world for the ingredients and claim them
function CraftOrder:search_for_ingredients()
   self._ingredient_list_object:search_for_ingredients()
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
   elseif self._condition.inventory_below > 0 then
      local craftable_tracker = object_tracker:get_craftable_tracker(self._faction)
      local target = self._recipe.produces[1].item
      local num_targets = craftable_tracker:get_quantity(target)
      if num_targets == nil then
         num_targets = 0
      end
      if num_targets < self._condition.inventory_below then
         return true
      end
      return false
   end
end

--- Check if the ingredients for an order are available in the inventory and workbench.
--  If so, return them. 
function CraftOrder:get_all_ingredients()
   return self._ingredient_list_object:get_all_ingredients()
end

--- Call whenever a part of an order is finished. (ie, built 2 of 4)
--   Used to determine if an order is fully complete. Inventory_below orders
--   are never complete, as the crafter is always monitoring if
--   they should be making more.
--   @returns: true if the order is complete and can be removed from
--             the list, false otherwise.
function CraftOrder:check_complete()
   self._ingredient_list_object = IngredientList(
      self._workshop:get_entity(), 
      self._workshop:get_items_on_bench(), 
      self._recipe.ingredients)

   self:set_crafting_status(false)

   --check if we're done with the whole order
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