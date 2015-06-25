local Entity = _radiant.om.Entity
local PickupItemTypeFromBackpackProxy = class()

PickupItemTypeFromBackpackProxy.name = 'pickup item type from backpack (proxy)'
PickupItemTypeFromBackpackProxy.does = 'stonehearth:pickup_item_type'
PickupItemTypeFromBackpackProxy.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromBackpackProxy.think_output = {
   item = Entity
}
PickupItemTypeFromBackpackProxy.version = 2
PickupItemTypeFromBackpackProxy.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromBackpackProxy)
            :execute('stonehearth:pickup_item_type_from_backpack', {
                  filter_fn = ai.ARGS.filter_fn,
                  description = ai.ARGS.description,
               })
            :set_think_output({
               item = ai.PREV.item
            })
