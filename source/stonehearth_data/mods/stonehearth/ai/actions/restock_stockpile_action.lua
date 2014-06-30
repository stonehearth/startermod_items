StockpileComponent = require 'components.stockpile.stockpile_component'

local RestockStockpile = class()
RestockStockpile.name = 'restock stockpile'
RestockStockpile.does = 'stonehearth:restock_stockpile'
RestockStockpile.args = {
   stockpile = StockpileComponent
}
RestockStockpile.version = 2
RestockStockpile.priority = 1

function RestockStockpile:start(ai, entity, args)
   ai:set_status_text('restocking ' .. radiant.entities.get_name(args.stockpile:get_entity()))
end

local ai = stonehearth.ai
return ai:create_compound_action(RestockStockpile)
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.item_filter,
            description = 'items to restock',
         })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.stockpile:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.stockpile:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
         -- Immediately after dropping the item into the stockpile, notify it that we've added something
         -- so it can update it's "valid spaces to drop stuff" region immediately.  Waiting for a 
         -- lua trace on the terrain to fire would allow additional code to run after we've unreserved the
         -- spot, but before we've marked it as occupied!
         :execute('stonehearth:call_method', {
            obj = ai.ARGS.stockpile,
            method = 'notify_restock_finished',
            args = { ai.BACK(2).location }
         })
