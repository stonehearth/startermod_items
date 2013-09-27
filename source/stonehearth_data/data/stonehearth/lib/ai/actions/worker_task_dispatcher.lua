local WorkerTaskDispatcher = class()

WorkerTaskDispatcher.name = 'worker task dispatcher'
WorkerTaskDispatcher.does = 'stonehearth.top'
WorkerTaskDispatcher.priority = 0


function WorkerTaskDispatcher:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   
   local faction = self._entity:get_component('unit_info'):get_faction()
   self._scheduler = radiant.mods.require('stonehearth.api').get_worker_scheduler(faction)

   self:_wait_for_next_task()
end

function WorkerTaskDispatcher:_wait_for_next_task()
   if self._task then
      self._scheduler:abort_worker_task(self._task)
      self._task = nil
   end

   local dispatch_fn = function(priority, packed_action)
      self._task = packed_action
      self._ai:set_action_priority(self, priority)
   end

   self._ai:set_action_priority(self, 0)
   self._scheduler:add_worker(self._entity, dispatch_fn)
end

function WorkerTaskDispatcher:run(ai, entity, ...)
   if self._task then
      ai:execute(unpack(self._task))
      self._task = nil
   else 
      radiant.log.warning('trying to dispatch a nil task in WorkerTaskDispatcher for %s', tostring(entity))
   end
end

function WorkerTaskDispatcher:stop()
   self:_wait_for_next_task()
end

return WorkerTaskDispatcher
