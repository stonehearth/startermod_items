local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local ChopTreeAction = class()

ChopTreeAction.name = 'chop tree'
ChopTreeAction.does = 'stonehearth:chop_tree'
ChopTreeAction.args = {
   tree = Entity      -- the tree to chop
}
ChopTreeAction.version = 2
ChopTreeAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(ChopTreeAction)
         :when( function (ai) return ai.CURRENT.carrying == nil end )
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.tree })
         :execute('stonehearth:chop_tree_adjacent', { tree = ai.ARGS.tree })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'chop_tree',
            },
         })
