local Entity = _radiant.om.Entity
local PickupItemTypeFromAnyStorage = class()

PickupItemTypeFromAnyStorage.name = 'pickup item type from any storage'
PickupItemTypeFromAnyStorage.does = 'stonehearth:pickup_item_type'
PickupItemTypeFromAnyStorage.args = {
   filter_fn = 'function',
   description = 'string',
}
PickupItemTypeFromAnyStorage.think_output = {
   item = Entity,          -- what actually got picked up
}
PickupItemTypeFromAnyStorage.version = 2
PickupItemTypeFromAnyStorage.priority = 1

function PickupItemTypeFromAnyStorage:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupItemTypeFromAnyStorage)
         :execute('stonehearth:find_path_to_storage_containing_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            description = ai.ARGS.description,
         })
         :execute('stonehearth:pickup_item_type_from_storage', {
            filter_fn = ai.ARGS.filter_fn,
            storage = ai.PREV.destination,
            description = ai.ARGS.description,
         })
         :set_think_output({ 
            item = ai.PREV.item,
         })
         