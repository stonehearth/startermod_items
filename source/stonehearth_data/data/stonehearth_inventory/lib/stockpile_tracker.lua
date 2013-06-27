local StockpileTracker = class()

function StockpileTracker:__init(inventory, stockpile_entity)
   self._inventory = inventory
   self._item_drop_locations = {}
   
   local solved = function(path)
      -- we found a backpath to an item!  get the item, and register it
      -- with the inventory system as an item which we can restock
      local item_entity = path:get_destination()
      local drop_location = path:get_start_point()
      
      inventory:_register_restock_item(item_entity, path)
      self._item_drop_locations[item_entity:get_id()] = drop_location
   end
   
   -- start finding back-paths to all the items this stockpile can
   -- store.
   local filter = nil
   inventory:find_backpath_to_item(stockpile_entity, solved)
end

function StockpileTracker:untrack_item(item_entity_id)
   local drop_location = self._item_drop_locations[item_entity_id]
   if drop_location then
      -- hmm... maybe the item went away because a guy is coming
      -- to drop it off in the stockpile.  what now?  we can't
      -- remove the registration.... hrmmmmmm.
   end
end

return StockpileTracker

