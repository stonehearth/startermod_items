local constants = require 'constants'

local DefendTaskDispatcher = class()
DefendTaskDispatcher.name = 'defend'
DefendTaskDispatcher.does = 'stonehearth:top'
DefendTaskDispatcher.args = {}
DefendTaskDispatcher.version = 2
DefendTaskDispatcher.priority = constants.priorities.top.DEFEND

local ai = stonehearth.ai
return ai:create_compound_action(DefendTaskDispatcher)
         :execute('stonehearth:defend')
