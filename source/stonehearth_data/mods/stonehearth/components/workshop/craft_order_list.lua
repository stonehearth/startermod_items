--[[
   Contains all the data for the todo list of a workshop.
   The player places craft orders, orders them, and can enable/disable them.
   A crafter will execute the craft orders in a todo list from top to bottom.
]]

local CraftOrder = require 'components.workshop.craft_order'

local CraftOrderList = class()
function CraftOrderList:__init()
   self._orders = {}
   self.__savestate = radiant.create_datastore({
         orders = self._orders,
      })
end

function CraftOrderList:is_paused()
   return self._is_paused
end

function CraftOrderList:toggle_pause()
   self._is_paused = not self._is_paused
   self.__savestate:modify_data(function (data)
         data.is_paused = self._is_paused
      end)
   self:_on_order_list_changed()
end

--[[
   Add an order to the list
   order:         A table, as defined below, with the values for the order
   target_index:  Optional index to add order. 1 is the first location.
                  If no index is selected, the order will be added to the end of the array
]]
function CraftOrderList:add_order(recipe, condition, faction)
   local order = CraftOrder(recipe, condition, faction, function()
         self:_on_order_list_changed()
      end)
   table.insert(self._orders, order)
   self:_on_order_list_changed()
end

--[[
   Iterate through the list from top to bottom. Check for the first enabled,
   relevant craft_order and return it and the paths associated with its
   ingredients. Mark that order as the top task.
   return: nil if the list is empty.
]]
function CraftOrderList:get_next_order()
   for i, order in ipairs(self._orders) do
      if order:should_execute_order() then
         return order
      end
   end
end

--[[
   Remove the object with the ID and insert it into the new positon.
   Note: do not call existing add/remove or we'll have multiple UI updates.
]]
function CraftOrderList:change_order_position(new, id)
   local i = self:_find_index_of(id)
   local order = self._orders[i]
   table.remove(self._orders, i)
   table.insert(self._orders, new, order)
   --TODO: comment out when you've fixed the drag/drop problem
   self:_on_order_list_changed()
end

function CraftOrderList:remove_order(order)
   return self:remove_order_id(order:get_id())
end

--[[
   Remove a craft_order given its ID. For example, use
   if the ui removes an order.
   order_id: the unique ID that represents this order
   returns:  the craft_order to remove or nil if the order
             can't be found.
]]
function CraftOrderList:remove_order_id(order_id)
    local i = self:_find_index_of(order_id)
    if i then
      local order = self._orders[i]
      table.remove(self._orders, i)
      order:destroy()
      self:_on_order_list_changed()
   end
end

-- Helper functions

--[[
   Find a craft_order by its ID.
   order_id: the unique ID that represents this order
   returns:  the craft_order associated with the ID or nil if the order
             cannot be found
]]
function CraftOrderList:_find_index_of(order_id)
   for i, order in ipairs(self._orders) do
      if order:get_id() == order_id then
         return i
      end
   end
   -- If it can't find the order, the user probably deleted the order out of the queue
   return nil
end

function CraftOrderList:_on_order_list_changed()
   radiant.events.trigger(self, 'order_list_changed')
   self.__savestate:mark_changed()
end


return CraftOrderList