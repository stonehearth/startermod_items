local Entity = _radiant.om.Entity
StockpileComponent = require 'components.stockpile.stockpile_component'

local MoveItemToOutbox = class()
MoveItemToOutbox.name = 'move item to outbox'
MoveItemToOutbox.does = 'stonehearth:move_item_to_outbox'
MoveItemToOutbox.args = {
   outbox = StockpileComponent,
   item = Entity,
}
MoveItemToOutbox.version = 2
MoveItemToOutbox.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(MoveItemToOutbox)
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.outbox })
         :execute('stonehearth:pickup_item_on_table', { item = ai.ARGS.item })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.outbox:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.outbox:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
