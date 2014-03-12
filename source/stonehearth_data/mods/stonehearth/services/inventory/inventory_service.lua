local Inventory = require 'services.inventory.inventory'

local InventoryService = class()

function InventoryService:__init()
   self._inventories = {}
end

function InventoryService:initialize()   
   self.__saved_variables = radiant.create_datastore({
         inventories = self._inventories
      })
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

function InventoryService:get_inventory(faction)
   radiant.check.is_string(faction)
   local inventory = self._inventories[faction]
   if not inventory then
      inventory = Inventory(faction)
      inventory:initialize()
      self._inventories[faction] = inventory
      self.__saved_variables:mark_changed()
   end
   return inventory
end

return InventoryService

