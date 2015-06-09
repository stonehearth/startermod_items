local Entity = _radiant.om.Entity

local FillCrate = class()
FillCrate.name = 'fill crate'
FillCrate.does = 'stonehearth:fill_crate'
FillCrate.args = {
   crate = Entity
}
FillCrate.version = 2
FillCrate.priority = 1

function FillCrate:start(ai, entity, args)
   ai:set_status_text('filling crate ' .. radiant.entities.get_name(args.crate))
end

local ai = stonehearth.ai
return ai:create_compound_action(FillCrate)
         :execute('stonehearth:wait_for_crate_space', { crate = ai.ARGS.crate })
         :execute('stonehearth:pickup_item_type', {
            filter_fn = ai.PREV.item_filter,
            description = 'items to restock',
         })
         :execute('stonehearth:drop_carrying_in_crate', { crate = ai.ARGS.crate })
