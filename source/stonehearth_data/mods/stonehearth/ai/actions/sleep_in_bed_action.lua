local RestockStockpileAction = class()
local StockpileComponent = require 'components.stockpile.stockpile_component'

RestockStockpileAction.name = 'restock stockpile'
RestockStockpileAction.does = 'stonehearth:restock_stockpile'
RestockStockpileAction.args = {
   stockpile = StockpileComponent      -- the stockpile that needs stuff
}
RestockStockpileAction.run_args = {
   stockpile = StockpileComponent,     -- the stockpile that needs stuff
   item_filter = 'function'            -- the filter function for stockpile items
}
RestockStockpileAction.version = 2
RestockStockpileAction.priority = 1

function RestockStockpileAction:start_thinking(ai, entity, stockpile)
   self._ai = ai
   radiant.events.listen(stockpile, 'space_available', self, self._on_space_available)
   self:_on_space_available(not stockpile:is_full())
end

function RestockStockpileAction:_on_space_available(space_available)
   if space_available then
      self._ai:set_run_args(stockpile, stockpile:get_item_filter_fn)
   else
      self._ai:revoke_run_args()
   end
end

function RestockStockpileAction:stop_thinking(ai, entity)
   radiant.events.unlisten(stockpile, 'space_available', self, self._on_space_available)
end

local ai = stonehearth.ai
return ai.create_compound_action(RestockStockpileAction)
         :execute('stonehearth:find_path_to_entity_type', { filter_fn = ai.PREV.item_filter })