local constants = require 'constants'

local FarmerTaskDispatcher = class()
FarmerTaskDispatcher.name = 'do farming'
FarmerTaskDispatcher.does = 'stonehearth:work'
FarmerTaskDispatcher.args = {}
FarmerTaskDispatcher.version = 2
FarmerTaskDispatcher.priority = constants.priorities.work.FARMING

local ai = stonehearth.ai
return ai:create_compound_action(FarmerTaskDispatcher)
         :execute('stonehearth:farm')
