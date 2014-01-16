local WorkerTaskDispatcher = class()
WorkerTaskDispatcher.name = 'do work'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.args = {}
WorkerTaskDispatcher.version = 2
WorkerTaskDispatcher.priority = 3

function WorkerTaskDispatcher:__init(ai, entity)
   self._entity = entity
   stonehearth.tasks:get_scheduler('stonehearth:workers')
                        :join(entity)
end

function WorkerTaskDispatcher:destroy()
   stonehearth.tasks:get_scheduler('stonehearth:workers')
                        :leave(self._entity)
end

-- wire stonehearth:work into the decision tree for all stonehearth:workers
 stonehearth.tasks:get_scheduler('stonehearth:workers')
                  :set_activity('stonehearth:work')

local ai = stonehearth.ai
return ai:create_compound_action(WorkerTaskDispatcher)
         :execute('stonehearth:work')
