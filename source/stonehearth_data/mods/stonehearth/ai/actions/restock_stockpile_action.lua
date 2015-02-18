local Entity = _radiant.om.Entity

local RestockStockpile = class()
RestockStockpile.name = 'restock stockpile'
RestockStockpile.does = 'stonehearth:restock_stockpile'
RestockStockpile.args = {
   stockpile = Entity
}
RestockStockpile.version = 2
RestockStockpile.priority = 1

function RestockStockpile:start(ai, entity, args)
   ai:set_status_text('restocking ' .. radiant.entities.get_name(args.stockpile))
end

local ai = stonehearth.ai
return ai:create_compound_action(RestockStockpile)
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.stockpile })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.item_filter,
            description = 'items to restock',
         })
         :execute('stonehearth:drop_carrying_in_stockpile', { stockpile = ai.ARGS.stockpile })
