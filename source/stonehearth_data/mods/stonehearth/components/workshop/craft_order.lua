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

--[[
Create a new CraftOrder
   recipe:     The name of the thing to build in mod:// format
   enabled:    Whether or not the user currently wants the crafter to
               evaluate the order
   condition:  The parameters that must be met for the carpenter to make
               the order.
   ingredients:TODO: the ingredients chosen by the user for the object
]]

function CraftOrder:initialize(id, recipe, condition, player_id, order_list)
   assert(self._sv)
   
   self._sv.id = id
   self._sv.recipe = recipe
   self._sv.portrait = recipe.portrait
   self._sv.condition = condition
   self._sv.enabled = true
   self._sv.is_crafting = true
   self._sv.order_list = order_list
   self._sv.player_id = player_id

   local condition = self._sv.condition
   if condition.type == "make" then
      condition.amount = tonumber(condition.amount)
      condition.remaining = condition.amount
   elseif condition.type == "maintain" then
      condition.at_least = tonumber(condition.at_least)
   end
   self:_on_changed()
end

function CraftOrder:_on_changed()
   self.__saved_variables:mark_changed()
   self._sv.order_list:_on_order_list_changed()
end

--[[
   Destructor??
]]
function CraftOrder:destroy()
   self:_on_changed()
end

-- Getters and Setters
function CraftOrder:get_id()
   return self._sv.id
end

function CraftOrder:get_recipe()
   return self._sv.recipe
end

function CraftOrder:get_enabled()
   return self._sv.enabled
end

function CraftOrder:toggle_enabled()
   self._sv.enabled = not self._sv.enabled
   self:_on_changed()
end

function CraftOrder:get_condition()
   return self._sv.condition
end

function CraftOrder:set_crafting_status(status)
   if status ~= self._sv.is_crafting then
      self._sv.is_crafting = status
      self:_on_changed()
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
   local condition = self._sv.condition
   if condition.type == "make" then
      return condition.remaining > 0 
   elseif condition.type == "maintain" then
      local we_have = 0
      local inventory_data_for_item = stonehearth.inventory:get_items_of_type(self._sv.recipe.produces[1].item, self._sv.player_id)
      if inventory_data_for_item and inventory_data_for_item.count then
         we_have = inventory_data_for_item.count
      end
      return we_have < condition.at_least
   end
end

function CraftOrder:on_item_created()
   local condition = self._sv.condition
   if condition.type == "make" then
      condition.remaining = condition.remaining - 1
      self:_on_changed()
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
   local condition = self._sv.condition
   if condition.type == "make" then
      return condition.remaining == 0
   elseif condition.type == "maintain" then
      return false
   end

end

return CraftOrder