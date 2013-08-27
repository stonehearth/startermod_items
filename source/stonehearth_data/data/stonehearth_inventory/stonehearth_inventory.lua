local Inventory = radiant.mods.require('/stonehearth_inventory/lib/inventory.lua')

local stonehearth_inventory = {}
local singleton = {}

function stonehearth_inventory.__init()
   singleton._inventories = {}
   singleton._stockpiles = {}
end

function stonehearth_inventory.get_inventory(faction)
   radiant.check.is_string(faction)
   local inventory = singleton._inventories[faction]
   if not inventory then
      inventory = Inventory(faction)
      singleton._inventories[faction] = inventory
   end
   return inventory
end

function stonehearth_inventory.register_stockpile(stockpile)
   singleton._stockpiles[stockpile:get_id()] = stockpile
end

function stonehearth_inventory.unregister_stockpile(id)
   singleton._stockpiles[id] = nil
end

function stonehearth_inventory.enumerate_items(faction, cb)
   local notified = {}
   
   for id, stockpile in pairs(singleton._stockpiles) do
      for item_id, item in pairs(stockpile:get_items()) do
         if not notified[item_id] then
            notified[item_id] = true
            cb(notified)
         end
      end
   end
end

stonehearth_inventory.__init()
return stonehearth_inventory
