local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local MoveUnit = class()

MoveUnit.name = 'move unit'
MoveUnit.does = 'stonehearth:unit_control:move_unit'
MoveUnit.args = {
   location = Point3      -- where to move
}

MoveUnit.version = 2
MoveUnit.priority = stonehearth.constants.priorities.unit_control.MOVE

local ai = stonehearth.ai
return ai:create_compound_action(MoveUnit)
         :execute('stonehearth:goto_location', { location = ai.ARGS.location  })
