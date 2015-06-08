local Entity = _radiant.om.Entity
local PickupItemTypeFromContainer = class()

PickupItemTypeFromContainer.name = 'pickup item type from container'
PickupItemTypeFromContainer.does = 'stonehearth:pickup_item_type'
PickupItemTypeFromContainer.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromContainer.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemTypeFromContainer.version = 2
PickupItemTypeFromContainer.priority = 1

function PickupItemTypeFromContainer:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromContainer)
         :execute('stonehearth:find_path_to_container_with_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            description = ai.ARGS.description,
         })
         :execute('stonehearth:find_backpack_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            backpack_entity = ai.PREV.destination,
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.BACK(2).destination,
         })
         :execute('stonehearth:reserve_entity', { 
            entity = ai.BACK(2).item,
         })
         :execute('stonehearth:pickup_item_from_backpack', { 
            item = ai.BACK(3).item,
            backpack_entity = ai.BACK(4).destination,
         })
         :set_think_output({ 
            item = ai.PREV.item
         })
         