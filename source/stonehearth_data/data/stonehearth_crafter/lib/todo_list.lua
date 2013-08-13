--[[
   Contains all the data for the todo list of a workshop.
   The player places craft orders, orders them, and can enable/disable them.
   A crafter will execute the craft orders in a todo list from top to bottom.
]]

local CraftOrder = radiant.mods.require('/stonehearth_crafter/lib/craft_order.lua')
local ToDoList = class()

function ToDoList:__init(data_blob)
   self._data_blob = data_blob
   self._my_list = {}
   self._is_paused = false
end

function ToDoList:__tojson()
   return radiant.json.encode(self._my_list)
end

function ToDoList:togglePause()
   self._is_paused = not self._is_paused
   self._backing_obj:mark_changed()
end

function ToDoList:is_paused()
   return self._is_paused
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
   self._data_blob:mark_changed()
end

--[[
   Iterate through the list from top to bottom. Check for the first enabled,
   relevant craft_order and return it and the paths associated with its
   ingredients. Mark that order as the top task.
   return: nil if the list is empty.
]]
function ToDoList:get_next_task()
   for i, order in ipairs(self._my_list) do
      if order:should_execute_order() then
         order:search_for_ingredients()
         -- TODO: need a way for the order to say "no, i can't go yet... move onto
         -- the next guy".  this should happens when the search for an ingredient
         -- gets exhausted. Should also return reasons for failure (can't find wood)
         local ingredients = order:get_all_ingredients()
         if ingredients then
            order:set_crafting_status(true)
            self._backing_obj:mark_changed()
            return order, ingredients
         end
         return
      end
   end
end

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
   --TODO: updates whole list when any object order is updated. Add backing object to TODO item?
   self._backing_obj:mark_changed()
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
   Remove the object with the ID and insert it into the new positon.
   Note: do not call existing add/remove or we'll have multiple UI updates.
]]
function ToDoList:change_order_position(new, id)
   local i = self:find_index_of(id)
   local order = self._my_list[i]
   table.remove(self._my_list, i)
   table.insert(self._my_list, new, order)
   --TODO: comment out when you've fixed the drag/drop problem
   self._backing_obj:mark_changed()
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
      order:destroy()
      self._data_blob:mark_changed()
   else
      return nil
>>>>>>> c703bd606c10242996069770eb1551982ddc4ac9
   end
end


return ToDoList