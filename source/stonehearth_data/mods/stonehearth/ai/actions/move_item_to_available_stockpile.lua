local Entity = _radiant.om.Entity
StockpileComponent = require 'components.stockpile.stockpile_component'

local MoveItemToAvailableStockpile = class()
MoveItemToAvailableStockpile.name = 'move item to available stockpile'
MoveItemToAvailableStockpile.does = 'stonehearth:clear_workshop'
MoveItemToAvailableStockpile.args = {
   item = Entity,
}
MoveItemToAvailableStockpile.version = 2
MoveItemToAvailableStockpile.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(MoveItemToAvailableStockpile)
         :execute('stonehearth:wait_for_closest_stockpile_space', { item = ai.ARGS.item })
         :execute('stonehearth:pickup_item_on_table', { item = ai.ARGS.item })
         :execute('stonehearth:goto_entity', { entity = ai.BACK(2).stockpile })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.BACK(3).stockpile,
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
