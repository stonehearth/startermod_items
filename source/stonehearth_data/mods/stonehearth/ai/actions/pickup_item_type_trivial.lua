local Entity = _radiant.om.Entity
local PickupItemTypeTrivial = class()

PickupItemTypeTrivial.name = 'pickup item trivial'
PickupItemTypeTrivial.does = 'stonehearth:pickup_item_type'
PickupItemTypeTrivial.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeTrivial.think_output = {
   item = Entity
}
PickupItemTypeTrivial.version = 2
PickupItemTypeTrivial.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeTrivial)
         :when( function (ai, entity, args)
               return ai.CURRENT.carrying ~= nil and args.filter_fn(ai.CURRENT.carrying, entity)
            end )
         :set_think_output({ item = ai.CURRENT.carrying })
         