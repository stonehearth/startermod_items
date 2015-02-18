local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropCarryingInStockpile = class()
DropCarryingInStockpile.name = 'drop carrying in stockpile'
DropCarryingInStockpile.does = 'stonehearth:drop_carrying_in_stockpile'
DropCarryingInStockpile.args = {
   stockpile = Entity,
}
DropCarryingInStockpile.version = 2
DropCarryingInStockpile.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingInStockpile)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.stockpile })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.stockpile,
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:drop_carrying_adjacent', { location = ai.PREV.location })
         -- Immediately after dropping the item into the stockpile, notify it that we've added something
         -- so it can update it's "valid spaces to drop stuff" region immediately.  Waiting for a 
         -- lua trace on the terrain to fire would allow additional code to run after we've unreserved the
         -- spot, but before we've marked it as occupied!
         :execute('stonehearth:call_method', {
            obj = ai.ARGS.stockpile:get_component('stonehearth:stockpile'),
            method = 'notify_restock_finished',
            args = { ai.BACK(2).location }
         })
