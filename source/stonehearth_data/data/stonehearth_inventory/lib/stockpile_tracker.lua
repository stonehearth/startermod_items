local StockpileTracker = class()

function StockpileTracker:__init(inventory, stockpile_entity)
   self._inventory = inventory
   self._item_drop_locations = {}
   self._stockpile_component = stockpile_entity:get_component('radiant:stockpile') 
   
   local solved = function(path)
      -- we found a backpath to an item!  get the item, and register it
      -- with the inventory system as an item which we can restock
      local item_entity = path:get_source()
      local drop_location = path:get_finish_point()
      
      inventory:_register_restock_item(item_entity, path)
      self._item_drop_locations[item_entity:get_id()] = drop_location
      
      -- at this point, we need to hold onto a reservation, remove this
      -- location from the region of the stockpile, etc.
      self._stockpile_component:reserve_restock_location(drop_location)
   end
   
   -- start finding back-paths to all the items this stockpile can
   -- store.
   local filter = function (entity)
      --radiant.log.warning('in filter function for %s', tostring(entity))
      local item = entity:get_component('item')
      if item then
         return not inventory:is_item_stocked(entity)
      end
      return false
   end
   
   inventory:find_backpath_to_item(stockpile_entity, solved, filter)
end

function StockpileTracker:contains_item(item_entity)
   return self._stockpile_component:contains(item_entity)
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

