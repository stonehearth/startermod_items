local Entity = _radiant.om.Entity
local ClearItem = class()

ClearItem.name = 'clear item'
ClearItem.does = 'stonehearth:clear_item'
ClearItem.args = {
   item = Entity
}
ClearItem.version = 2
ClearItem.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(ClearItem)
   :execute('stonehearth:drop_carrying_now', {})
   :execute('stonehearth:reserve_entity', { entity = ai.ARGS.item })
   :execute('stonehearth:goto_entity', { entity = ai.ARGS.item })
   :execute('stonehearth:clear_item_adjacent', {item = ai.ARGS.item})