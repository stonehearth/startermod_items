local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local ChopTreeAction = class()

ChopTreeAction.name = 'chop tree'
ChopTreeAction.does = 'stonehearth:chop_tree'
ChopTreeAction.args = {
   Entity      -- the tree to chop
}
ChopTreeAction.version = 2
ChopTreeAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(ChopTreeAction)
         :execute('stonehearth:goto_entity', ai.ARGS[1], 'run')
         :execute('stonehearth:chop_tree:adjacent', ai.ARGS[1])
