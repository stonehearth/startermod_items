--[[
   Contains all the data for the todo list of a workshop.
   The player places craft orders, orders them, and can enable/disable them.
   A crafter will execute the craft orders in a todo list from top to bottom.
]]

local CraftOrder = radiant.mods.require('mod://stonehearth_crafter/lib/craft_order.lua')
local ToDoList = class()

function ToDoList:__init()
   self._my_list = {}
end

--[[
   Add an order to the list
   order:         A table, as defined below, with the values for the order
   target_index:  Optional index to add order. 1 is the first location.
                  If no index is selected, the order will be added to the end of the array
]]
function ToDoList:add_order(order, target_index)
   if target_index then
      table.insert(self._my_list, target_index, order)
   else
      table.insert(self._my_list, order)
   end
end

--[[
   Iterate through the list from top to bottom. Check for the first enabled,
   relevant craft_order and return it and the paths associated with its
   ingredients.
   return: nil if the list is empty.
]]
function ToDoList:get_next_task()
   if table.getn(self._my_list) < 1 then
      return nil
   end
   local i = 1
   while (not self._my_list[i]:can_execute_order()) do
      i = i + 1;
   end
   --TODO: figure out inventory syntax. We need the ingredients relevant to this recipe, and paths to each

   --FOR TESTING ONLY
   local ingredient_data = {}
   local recipe = self._my_list[i]:get_recipe().produces[1].item
   if recipe == 'mod://stonehearth_items/wooden_buckler' then
      ingredient_data = self:buckler_ingredients()
   elseif recipe == 'mod://stonehearth_items/wooden_sword' then
      ingredient_data = self:sword_ingredients()
   end

   return self._my_list[i], ingredient_data
end

--TESTING FUNCTIONS ONLY
---[[
function ToDoList:sword_ingredients()
   local wood = radiant.entities.create_entity('mod://stonehearth_trees/entities/oak_tree/oak_log')
   radiant.terrain.place_entity(wood, RadiantIPoint3(-10, 1, 10))
   local wood2 = radiant.entities.create_entity('mod://stonehearth_trees/entities/oak_tree/oak_log')
   radiant.terrain.place_entity(wood2, RadiantIPoint3(-9, 1, 10))
   local wood3 = radiant.entities.create_entity('mod://stonehearth_trees/entities/oak_tree/oak_log')
   radiant.terrain.place_entity(wood3, RadiantIPoint3(-8, 1, 10))

   local ingredient_data = {{item = wood},{item = wood2},{item = wood3}}
   return ingredient_data
end

--TESTING FUNCTIONS ONLY
function ToDoList:buckler_ingredients()
   local wood = radiant.entities.create_entity('mod://stonehearth_trees/entities/oak_tree/oak_log')
   radiant.terrain.place_entity(wood, RadiantIPoint3(-10, 1, 10))

   local cloth = radiant.entities.create_entity('mod://stonehearth_items/cloth_bolt')
   radiant.terrain.place_entity(cloth, RadiantIPoint3(-7, 1, 10))

   local ingredient_data = {{item = wood}, {item = cloth}}
   return ingredient_data
end
--]]

--[[
   When the crafter as completed his current task, call this function
   to notify the todo list. If the craft_order in question is complete
   and should be removed from the list, do so.
]]
function ToDoList:chunk_complete(curr_order)
   -- Verify that the current order is still in the queue somewhere
   local i = self:find_index_of(curr_order:get_id())
   if i and curr_order:check_complete() then
      table.remove(self._my_list, i)
   end
end

-- Helper functions

--[[
   Find a craft_order by its ID.
   order_id: the unique ID that represents this order
   returns:  the craft_order associated with the ID or nil if the order
             cannot be found
]]
function ToDoList:find_index_of(order_id)
   for i, order in ipairs(self._my_list) do
      if order:get_id() == order_id then
         return i
      end
   end
   -- If it can't find the order, the user probably deleted the order out of the queue
   return nil
end


--[[
   Remove a craft_order given its ID. For example, use
   if the ui removes an order.
   order_id: the unique ID that represents this order
   returns:  the craft_order to remove or nil if the order
             can't be found.
]]
function ToDoList:remove_order(order_id)
    local i = self:find_index_of(order_id)
    if i then
      local order = self._my_list[i]
      table.remove(self._my_list, i)
      return order
   else
      return nil
   end
end


return ToDoList