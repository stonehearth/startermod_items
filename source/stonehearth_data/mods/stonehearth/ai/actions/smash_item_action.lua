local Entity = _radiant.om.Entity
local SmashItemAction = class()

SmashItemAction.name = 'smash an item'
SmashItemAction.does = 'stonehearth:smash_item'
SmashItemAction.args = {
   item = Entity,
}
SmashItemAction.version = 2
SmashItemAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(SmashItemAction)
         :execute('stonehearth:reserve_entity', { entity = ai.ARGS.item })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.item })
         :execute('stonehearth:smash_item_adjacent', { item = ai.ARGS.item })
