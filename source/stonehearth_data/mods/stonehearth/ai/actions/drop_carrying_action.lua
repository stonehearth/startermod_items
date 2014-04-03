local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local DropCarrying = class()

DropCarrying.name = 'drop carrying'
DropCarrying.does = 'stonehearth:drop_carrying'
DropCarrying.args = {
   location = Point3      -- where to drop it
}
DropCarrying.think_output = {
   item = Entity,          -- what got dropped
}
DropCarrying.version = 2
DropCarrying.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(DropCarrying)
         :execute('stonehearth:goto_location', {
            location = ai.ARGS.location,
            stop_when_adjacent = true,
         })
         :execute('stonehearth:drop_carrying_adjacent', {
            location = ai.ARGS.location
         })
         :set_think_output({
            item = ai.PREV.item
         })
