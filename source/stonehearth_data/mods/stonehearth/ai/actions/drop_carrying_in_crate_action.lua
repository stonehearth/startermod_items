local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropCarryingInCrate = class()
DropCarryingInCrate.name = 'drop carrying in crate'
DropCarryingInCrate.does = 'stonehearth:drop_carrying_in_crate'
DropCarryingInCrate.args = {
   crate = Entity,
}
DropCarryingInCrate.version = 2
DropCarryingInCrate.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingInCrate)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.crate })
         :execute('stonehearth:put_carrying_in_backpack', { backpack_entity = ai.ARGS.crate })
