StockpileComponent = require 'components.stockpile.stockpile_component'

local RestockStockpile = class()
RestockStockpile.name = 'restock stockpile'
RestockStockpile.does = 'stonehearth:restock_stockpile'
RestockStockpile.args = {
   stockpile = StockpileComponent
}
RestockStockpile.version = 2
RestockStockpile.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RestockStockpile)
         :execute('stonehearth:delay_thinking', { time = 250 })
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.item_filter,
            reconsider_event_name = 'stonehearth:reconsider_stockpile_item' 
         })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.stockpile:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.stockpile:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
