local Entity = _radiant.om.Entity
local PickupItemTypeFromAnyContainer = class()

PickupItemTypeFromAnyContainer.name = 'pickup item type from any container'
PickupItemTypeFromAnyContainer.does = 'stonehearth:pickup_item_type'
PickupItemTypeFromAnyContainer.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromAnyContainer.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemTypeFromAnyContainer.version = 2
PickupItemTypeFromAnyContainer.priority = 1

function PickupItemTypeFromAnyContainer:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end


local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromAnyContainer)
         :execute('stonehearth:find_path_to_container_with_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            description = ai.ARGS.description,
         })
         :execute('stonehearth:pickup_item_type_from_container', {
            filter_fn = ai.ARGS.filter_fn,
            backpack_entity = ai.PREV.destination,
            description = ai.ARGS.description,
         })
         :set_think_output({ 
            item = ai.PREV.item,
         })
         