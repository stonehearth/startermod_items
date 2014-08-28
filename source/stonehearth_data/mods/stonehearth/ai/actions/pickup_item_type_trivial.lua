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

function PickupItemTypeTrivial:start_thinking(ai, entity, args)
   local item = ai.CURRENT.carrying

   if item and args.filter_fn(item, entity) then
      ai:set_think_output({ item = item })
   end
end

return PickupItemTypeTrivial
