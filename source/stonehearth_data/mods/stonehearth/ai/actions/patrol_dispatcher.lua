local constants = require 'constants'

local PatrolDispatcher = class()

PatrolDispatcher.name = 'patrol dispatcher'
PatrolDispatcher.does = 'stonehearth:work'
PatrolDispatcher.args = {}
PatrolDispatcher.version = 2
PatrolDispatcher.priority = constants.priorities.work.PATROLLING

local ai = stonehearth.ai
return ai:create_compound_action(PatrolDispatcher)
   :execute('stonehearth:patrol')
