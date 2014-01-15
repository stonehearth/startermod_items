local Entity = _radiant.om.Entity
local PickupItemType = class()

PickupItemType.name = 'pickup item type'
PickupItemType.does = 'stonehearth:pickup_item_type'
PickupItemType.args = {
   filter_fn = 'function',
}
PickupItemType.version = 2
PickupItemType.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemType)
         :when( function (ai)
               return ai.CURRENT.carrying == nil
            end )
         :execute('stonehearth:goto_entity_type', { filter_fn = ai.ARGS.filter_fn })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.destination_entity })
         :execute('stonehearth:pickup_item_adjacent', { item = ai.PREV.entity })
         