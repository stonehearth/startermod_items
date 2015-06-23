local Entity = _radiant.om.Entity

local FillStorage = class()
FillStorage.name = 'fill storage'
FillStorage.does = 'stonehearth:fill_storage'
FillStorage.args = {
   storage = Entity
}
FillStorage.version = 2
FillStorage.priority = 1

function FillStorage:start(ai, entity, args)
   ai:set_status_text('filling ' .. radiant.entities.get_name(args.storage))
end

local ai = stonehearth.ai
return ai:create_compound_action(FillStorage)
         :execute('stonehearth:wait_for_storage_space', { storage = ai.ARGS.storage })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.item_filter,
            description = 'items to fill storage',
         })
         :execute('stonehearth:drop_carrying_in_storage', { storage = ai.ARGS.storage })
