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
   container = Entity,     -- where are we picking it up from
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
         :execute('stonehearth:reserve_backpack_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            backpack_entity = ai.PREV.destination,
         })
         :execute('stonehearth:goto_entity', {
            entity = ai.PREV.backpack_entity,
         })
         :execute('stonehearth:pickup_item_from_backpack', { 
            item = ai.BACK(2).entity,
            container = ai.PREV.entity
         })
         :set_think_output({ 
            item = ai.PREV.carrying,
            container = ai.PREV.container
         })
         