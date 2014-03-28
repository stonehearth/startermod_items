local constants = require 'constants'

local AttackTaskDispatcher = class()
AttackTaskDispatcher.name = 'attack'
AttackTaskDispatcher.does = 'stonehearth:top'
AttackTaskDispatcher.args = {}
AttackTaskDispatcher.version = 2
AttackTaskDispatcher.priority = constants.priorities.top.ATTACK

local ai = stonehearth.ai
return ai:create_compound_action(AttackTaskDispatcher)
         :execute('stonehearth:attack')
