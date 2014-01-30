local WorkerTaskDispatcher = class()
WorkerTaskDispatcher.name = 'do work'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.args = {}
WorkerTaskDispatcher.version = 2
WorkerTaskDispatcher.priority = 3

local ai = stonehearth.ai
return ai:create_compound_action(WorkerTaskDispatcher)
         :execute('stonehearth:work')
