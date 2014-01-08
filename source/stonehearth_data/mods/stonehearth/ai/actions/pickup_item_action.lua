local Entity = _radiant.om.Entity
local PickupItem = class()

PickupItem.name = 'pickup an item'
PickupItem.does = 'stonehearth:pickup_item'
PickupItem.args = {
   item = Entity,
}
PickupItem.version = 2
PickupItem.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(PickupItem)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.item  })
         :execute('stonehearth:pickup_item_adjacent', { item = ai.ARGS.item })
