local constants = require 'constants'

local MiningDispatcher = class()
MiningDispatcher.name = 'mining dispatcher'
MiningDispatcher.does = 'stonehearth:work'
MiningDispatcher.args = {}
MiningDispatcher.version = 2
MiningDispatcher.priority = constants.priorities.work.MINING

local ai = stonehearth.ai
return ai:create_compound_action(MiningDispatcher)
         :execute('stonehearth:mining')
