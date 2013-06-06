local Inventory = radiant.mods.require('mod://stonehearth_inventory/lib/inventory.lua')
local stonehearth_inventory = {}
local singleton = {}

function stonehearth_inventory.__init()
   singleton._inventories = {}
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

stonehearth_inventory.__init()

return stonehearth_inventory

