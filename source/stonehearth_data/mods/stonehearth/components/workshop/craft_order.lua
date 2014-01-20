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

function CraftOrder:__init(recipe, condition, faction, on_change_cb)
   self._id = craft_order_id
   craft_order_id = craft_order_id + 1

   self._enabled = true
   self._is_crafting = false
   self._recipe = recipe
   self._faction = faction
   self._on_change_cb = on_change_cb   

   self._condition = condition
   if self._condition.type == "make" then
      self._condition.amount = tonumber(self._condition.amount)
      self._condition.remaining = self._condition.amount
   elseif self._condition.type == "maintain" then
      self._condition.at_least = tonumber(self._condition.at_least)
   end
end

function CraftOrder:__tojson()
   local json = {
      id = self._id,
      recipe = self._recipe,
      condition = self._condition,
      enabled = self._enabled,
      portrait = self._recipe.portrait,
      is_crafting = self._is_crafting
   }
   return radiant.json.encode(json)
end

--[[
   Destructor??
]]
function CraftOrder:destroy()
   self._on_change_cb()
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
   self._on_change_cb()
end

function CraftOrder:get_condition()
   return self._condition
end

function CraftOrder:set_crafting_status(status)
   if status ~= self._is_crafting then
      self._is_crafting = status
      self._on_change_cb()
   end
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
   if self._condition.type == "make" then
      return self._condition.remaining > 0 
   elseif self._condition.type == "maintain" then
      local craftable_tracker = stonehearth.object_tracker:get_craftable_tracker(self._faction)
      local target = self._recipe.produces[1].item
      local num_targets = craftable_tracker:get_quantity(target)
      if num_targets == nil then
         num_targets = 0
      end
      return num_targets < self._condition.at_least
   end
end

function CraftOrder:on_item_created()
   if self._condition.type == "make" then
      self._condition.remaining = self._condition.remaining - 1
   end
end

--- Call whenever a part of an order is finished. (ie, built 2 of 4)
--   Used to determine if an order is fully complete. Inventory_below orders
--   are never complete, as the crafter is always monitoring if
--   they should be making more.
--   @returns: true if the order is complete and can be removed from
--             the list, false otherwise.
function CraftOrder:is_complete()
   --check if we're done with the whole order
   if self._condition.type == "make" then
      return self._condition.remaining == 0
   elseif self._condition.type == "maintain" then
      return false
   end

end

return CraftOrder