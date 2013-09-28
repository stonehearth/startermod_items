local Inventory = require 'services.inventory.inventory'

local InventoryService = class()

function InventoryService:__init()
   self._inventories = {}
   self._stockpiles = {}
end

function InventoryService:get_inventory(faction)
   radiant.check.is_string(faction)
   local inventory = self._inventories[faction]
   if not inventory then
      inventory = Inventory(faction)
      self._inventories[faction] = inventory
   end
   return inventory
end

function InventoryService:register_stockpile(stockpile)
   self._stockpiles[stockpile:get_id()] = stockpile
end

function InventoryService:unregister_stockpile(id)
   self._stockpiles[id] = nil
end

function InventoryService:enumerate_items(faction, cb)
   local notified = {}
   
   for id, stockpile in pairs(self._stockpiles) do
      for item_id, item in pairs(stockpile:get_items()) do
         if not notified[item_id] then
            notified[item_id] = true
            cb(notified)
         end
      end
   end
end

return InventoryService()
