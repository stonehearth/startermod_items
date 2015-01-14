local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DestroyItemsInStockpileAction = class()

DestroyItemsInStockpileAction.name = 'destroy items in stockpile'
DestroyItemsInStockpileAction.does = 'stonehearth:party:raid_stockpile'
DestroyItemsInStockpileAction.args = {
   party = 'table',
   stockpile = Entity,
}

DestroyItemsInStockpileAction.version = 2
DestroyItemsInStockpileAction.priority = 1


local function cheap_items(item)
   local net_worth = radiant.entities.get_entity_data(item, 'stonehearth:net_worth')
   if not net_worth then
      return true
   end
   return net_worth.value_in_gold < 50
end

local ai = stonehearth.ai
return ai:create_compound_action(DestroyItemsInStockpileAction)
         :execute('stonehearth:choose_item_in_stockpile', { stockpile = ai.ARGS.stockpile, filter_fn = cheap_items })
         :execute('stonehearth:smash_item', { item = ai.PREV.item })
