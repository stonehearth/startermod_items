local constants = require 'constants'

local CombatTaskDispatcher = class()
CombatTaskDispatcher.name = 'combat'
CombatTaskDispatcher.does = 'stonehearth:top'
CombatTaskDispatcher.args = {}
CombatTaskDispatcher.version = 2
CombatTaskDispatcher.priority = constants.priorities.top.COMBAT

local ai = stonehearth.ai
return ai:create_compound_action(CombatTaskDispatcher)
         :execute('stonehearth:combat')
