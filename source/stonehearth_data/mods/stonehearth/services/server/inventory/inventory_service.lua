local Entity = _radiant.om.Entity

local Inventory = require 'services.server.inventory.inventory'

--[[
   The inventory service tracks, per player, the things in their stockpiles. 
   Given a player_id, you can ask for their inventory.
   You can also create a filtered set of the inventory, via an inventory tracker.

   The default inventory tracker, created by the service for each player,
   sorts the contents of their inventory by uri. 
]]

local InventoryService = class()

function InventoryService:__init()
end

function InventoryService:initialize()   
   self._sv = self.__saved_variables:get_data()
   if not self._sv.inventories then
      self._sv.inventories = {}
   end
end

function InventoryService:add_inventory(player_id)
   radiant.check.is_string(player_id)

   assert(not self._sv.inventories[player_id])

   local inventory = radiant.create_controller('stonehearth:inventory', player_id)
   assert(inventory)
   
   self._sv.inventories[player_id] = inventory
   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(arg1)
   local player_id
   if radiant.util.is_a(arg1, 'string') then
      player_id = arg1
   elseif radiant.util.is_a(arg1, Entity) then
      player_id = radiant.entities.get_player_id(arg1)
   else
      error(string.format('unexpected value %s in get_inventory', radiant.util.tostring(player_id)))
   end
   radiant.check.is_string(player_id)
   
   if self._sv.inventories[player_id] then
      return self._sv.inventories[player_id]
   end
end

function InventoryService:get_item_tracker_command(session, response, tracker_name)
   local inventory = self:get_inventory(session.player_id)

   if inventory == nil then
      response:reject('there is no inventory for player ' .. session.player_id)
      return
   end

   return { tracker = inventory:get_item_tracker(tracker_name) }
end

return InventoryService
