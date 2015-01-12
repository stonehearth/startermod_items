local Point3 = _radiant.csg.Point3

local IdleInFormationAction = class()

IdleInFormationAction.name = 'party idle'
IdleInFormationAction.does = 'stonehearth:party:hold_formation_location'
IdleInFormationAction.args = {
   location = Point3    -- world location of formation position
}
IdleInFormationAction.version = 2
IdleInFormationAction.priority = stonehearth.constants.priorities.party.hold_formation.IDLE_IN_FORMATION

local ai = stonehearth.ai
return ai:create_compound_action(IdleInFormationAction)
         :execute('stonehearth:idle', { hold_position = true })
