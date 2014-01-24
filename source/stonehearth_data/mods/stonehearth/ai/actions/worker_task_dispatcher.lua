local WorkerTaskDispatcher = class()
WorkerTaskDispatcher.name = 'do work'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.args = {}
WorkerTaskDispatcher.version = 2
WorkerTaskDispatcher.priority = 3

function WorkerTaskDispatcher:__init(entity)
   self._entity = entity
   stonehearth.tasks:get_scheduler('stonehearth:workers', radiant.entities.get_faction(entity))
                        :join(entity)
end

function WorkerTaskDispatcher:destroy()
   stonehearth.tasks:get_scheduler('stonehearth:workers', radiant.entities.get_faction(entity))
                        :leave(self._entity)
end

local ai = stonehearth.ai
return ai:create_compound_action(WorkerTaskDispatcher)
         :execute('stonehearth:work')
