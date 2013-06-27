local RestockStockpileAction = class()

RestockStockpileAction.name = 'stonehearth.actions.restock_stockpile'
RestockStockpileAction.does = 'stonehearth.activities.top'
RestockStockpileAction.priority = 0


function RestockStockpileAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   
   local faction = self._entity:get_component('unit_info'):get_faction()
   self._inventory = radiant.mods.require('/stonehearth_inventory').get_inventory(faction)
   
   radiant.events.listen('radiant.events.gameloop', self)
end

-- xxx: the 'fire one when i'm constructed' pattern again...
RestockStockpileAction['radiant.events.gameloop'] = function(self)
   self:_start_search()
   radiant.events.unlisten('radiant.events.gameloop', self)
end
   

function RestockStockpileAction:_start_search()
   local faction = self._entity:get_component('unit_info'):get_faction()

   self._path = nil
   self._ai:set_action_priority(self, 0)
   
   local solved = function(item, path_to_item, path_to_stockpile, drop_location)
      self._item = item
      self._path_to_item = path_to_item
      self._path_to_stockpile = path_to_stockpile
      self._drop_location = drop_location
      self._ai:set_action_priority(self, 10)      
   end
   
   self._path_to_item, self._item, self._path_to_stockpile, self._drop_location = nil
   self._inventory:find_item_to_restock(self._entity, solved)
end

function RestockStockpileAction:run(ai, entity)
   ai:execute('stonehearth.activities.follow_path', self._path_to_item)
   ai:execute('stonehearth.activities.pickup_item', self._item)
   ai:execute('stonehearth.activities.follow_path', self._path_to_stockpile)
   ai:execute('stonehearth.activities.drop_carrying', self._drop_location)
end

function RestockStockpileAction:stop()
   self:_start_search()
end

return RestockStockpileAction
