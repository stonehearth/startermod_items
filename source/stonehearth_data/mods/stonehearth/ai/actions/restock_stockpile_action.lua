local RestockStockpileAction = class()
StockpileComponent = require 'components.stockpile.stockpile_component'

RestockStockpileAction.name = 'restock stockpile'
RestockStockpileAction.does = 'stonehearth:restock_stockpile'
RestockStockpileAction.args = {
   stockpile = StockpileComponent
}
RestockStockpileAction.version = 2
RestockStockpileAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RestockStockpileAction)
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.PREV.item_filter })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.stockpile:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.stockpile:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
