local Entity = _radiant.om.Entity
local PickupItemType = class()

PickupItemType.name = 'pickup item type'
PickupItemType.does = 'stonehearth:pickup_item_type'
PickupItemType.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemType.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemType.version = 2
PickupItemType.priority = 1

function PickupItemType:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemType)
         :execute('stonehearth:goto_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            description = ai.ARGS.description,
         })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.destination_entity })
         :execute('stonehearth:pickup_item_adjacent', { item = ai.PREV.entity })
         :set_think_output({ item = ai.PREV.item })
         