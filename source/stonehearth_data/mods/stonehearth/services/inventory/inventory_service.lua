local Inventory = require 'services.inventory.inventory'

local InventoryService = class()

function InventoryService:__init()
   self._inventories = {}
end

function InventoryService:initialize()   
   local data = self.__saved_variables:get_data()
   if data.inventories then
      for faction, ss in pairs(data.inventories) do
         local inventory = Inventory(faction)
         inventory:restore(ss)
         self._inventories[faction] = inventory
      end      
   else
      self._inventories = {}
      data.inventories = self._inventories
   end
end

function InventoryService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self.__saved_variables:read_data(function(o)
         for faction, ss in pairs(o.inventories) do
            local inventory = Inventory(faction)
            inventory:restore(ss)
            self._inventories[faction] = inventory
         end
      end)
end

function InventoryService:add_inventory(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._inventories[session.player_id])

   local inventory = Inventory()
   inventory:initialize(session)
   self._inventories[session.player_id] = inventory
   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(player_id)
   radiant.check.is_string(player_id)
   return self._inventories[player_id]
end

return InventoryService

