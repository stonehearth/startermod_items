local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropItemNearWorkshop = class()
DropItemNearWorkshop.name = 'drop item near workshop'
DropItemNearWorkshop.does = 'stonehearth:clear_workshop'
DropItemNearWorkshop.status_text_key = 'ai_status_text_drop_item_near_workshop'
DropItemNearWorkshop.args = {
   item = Entity
}
DropItemNearWorkshop.version = 2
DropItemNearWorkshop.priority = 1 --Lower priority than dropping the item off at a nearby stockpile

local ai = stonehearth.ai
return ai:create_compound_action(DropItemNearWorkshop)
   :execute('stonehearth:pickup_item_on_table', { item = ai.ARGS.item })
   :execute('stonehearth:walk_to_empty_space', {radius = 8, radius_min = 1})
   :execute('stonehearth:drop_carrying_now')
