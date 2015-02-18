local entity_forms = require 'stonehearth.lib.entity_forms.entity_forms_lib'

local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local StealItemsInStockpileAction = class()

StealItemsInStockpileAction.name = 'steal items in stockpile'
StealItemsInStockpileAction.does = 'stonehearth:steal_items_in_stockpile'
StealItemsInStockpileAction.args = {
   from_stockpile = Entity,   -- the stockpile to raid
   to_stockpile = Entity,     -- the stockpile to take things to
}

StealItemsInStockpileAction.version = 2
StealItemsInStockpileAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(StealItemsInStockpileAction)
         :execute('stonehearth:wait_for_stockpile_space', { stockpile = ai.ARGS.to_stockpile })
         :execute('stonehearth:choose_item_in_stockpile', {
               stockpile = ai.ARGS.from_stockpile
            })
         :execute('stonehearth:pickup_item', { item = ai.PREV.item })
         :execute('stonehearth:drop_carrying_in_stockpile', { stockpile = ai.ARGS.to_stockpile })
