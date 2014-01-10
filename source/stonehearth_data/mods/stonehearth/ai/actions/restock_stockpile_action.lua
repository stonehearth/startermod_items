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
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.PREV.item_filter })
         :execute('stonehearth:find_path_to_entity', { finish = ai.ARGS.stockpile:get_entity(), reserve_region = true })
         :execute('stonehearth:follow_path', { path = ai.PREV.path })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.BACK(2).path:get_destination_point_of_interest() })
