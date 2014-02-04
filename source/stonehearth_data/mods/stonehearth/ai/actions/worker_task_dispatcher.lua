local constants = require 'constants'

local WorkerTaskDispatcher = class()
WorkerTaskDispatcher.name = 'do work'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.args = {}
WorkerTaskDispatcher.version = 2
WorkerTaskDispatcher.priority = constants.priorities.top.WORK

local ai = stonehearth.ai
return ai:create_compound_action(WorkerTaskDispatcher)
         :execute('stonehearth:work')
