local RestockStockpileAction = class()
local StockpileComponent = require 'components.stockpile.stockpile_component'

RestockStockpileAction.name = 'restock stockpile'
RestockStockpileAction.does = 'stonehearth:restock_stockpile'
RestockStockpileAction.args = {
   stockpile = StockpileComponent,      -- the stockpile that needs stuff
}
RestockStockpileAction.version = 2
RestockStockpileAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RestockStockpileAction)
         :execute('stonehearth:reserve_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.PREV.item_filter })
         :execute('stonehearth:drop_carrying', { location = ai.BACK(2).reserved_location })
