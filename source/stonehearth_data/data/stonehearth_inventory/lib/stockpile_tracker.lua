local StockpileTracker = class()

function StockpileTracker:__init(inventory, stockpile_entity)
   self._inventory = inventory

   local solved = function(path)
      local item_entity = path:get_destination()
      -- xxx: reserve spot in stockpile...
      inventory:_register_restock_item(item_entity, path)
   end
   
   local filter = nil
   inventory:find_backpath_to_item(stockpile_entity, solved)
end

return StockpileTracker
