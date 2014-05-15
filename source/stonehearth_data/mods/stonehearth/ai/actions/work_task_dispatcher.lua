local constants = require 'constants'

local WorkTaskDispatcher = class()
WorkTaskDispatcher.name = 'do work'
WorkTaskDispatcher.does = 'stonehearth:top'
WorkTaskDispatcher.args = {}
WorkTaskDispatcher.version = 2
WorkTaskDispatcher.priority = constants.priorities.top.WORK

local ai = stonehearth.ai
return ai:create_compound_action(WorkTaskDispatcher)
         :execute('stonehearth:work')
