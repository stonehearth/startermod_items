local constants = require 'constants'

local UnloadBackpackDispatcher = class()
UnloadBackpackDispatcher.name = 'unload backpack dispatcher'
UnloadBackpackDispatcher.does = 'stonehearth:trapping'
UnloadBackpackDispatcher.args = {}
UnloadBackpackDispatcher.version = 2
UnloadBackpackDispatcher.priority = constants.priorities.trapping.UNLOAD_BACKPACK

local ai = stonehearth.ai
return ai:create_compound_action(UnloadBackpackDispatcher)
         :execute('stonehearth:trapping:unload_backpack')
