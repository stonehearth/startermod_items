local Entity = _radiant.om.Entity

local DropItemNearWorkshop = class()
DropItemNearWorkshop.name = 'drop item near workshop'
DropItemNearWorkshop.does = 'stonehearth:clear_workshop'
DropItemNearWorkshop.args = {
   item = Entity
}
DropItemNearWorkshop.version = 2
DropItemNearWorkshop.priority = 1 --Lower priority than dropping the item off at a nearby stockpile

function DropItemNearWorkshop:start(ai, entity, args)
   ai:set_status_text('clearing workshop')
end

local ai = stonehearth.ai
return ai:create_compound_action(DropItemNearWorkshop)
   :execute('stonehearth:pickup_item_on_table', { item = ai.ARGS.item })
   :execute('stonehearth:wander', { radius_min = 1, radius = 3 })
   :execute('stonehearth:drop_carrying_now', {})