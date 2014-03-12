local Inventory = require 'services.inventory.inventory'

local InventoryService = class()

function InventoryService:__init()
   self._inventories = {}
end

function InventoryService:initialize()   
   self.__savestate = radiant.create_datastore({
         inventories = self._inventories
      })
end

function InventoryService:restore(savestate)
   self.__savestate = savestate
   self.__savestate:read_data(function(o)
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
      inventory:create()
      self._inventories[faction] = inventory
      self.__savestate:mark_changed()
   end
   return inventory
end

return InventoryService

