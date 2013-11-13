local WorkerTaskDispatcher = class()

WorkerTaskDispatcher.name = 'worker task dispatcher'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.priority = 0


function WorkerTaskDispatcher:__init(ai, entity)
   self._ai = ai
   self._entity = entity

   local faction = self._entity:get_component('unit_info'):get_faction()
   local ws = radiant.mods.load('stonehearth').worker_scheduler
   self._scheduler = ws:get_worker_scheduler(faction)

   self:_wait_for_next_task()
end

function WorkerTaskDispatcher:destroy()
   self._scheduler:remove_worker(self._entity)
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
   self._scheduler:remove_worker(self._entity)
   assert(self._task, "worker dispatcher has no task to run")
   
   
   --TODO: put this up over thier heads, like dialog!
   local name = entity:get_component('unit_info'):get_display_name()
   radiant.log.info('Worker %s: Hey! About to %s!', name, self._task[1])
   
   ai:execute(unpack(self._task))
   self._task = nil
end

function WorkerTaskDispatcher:stop()
   self:_wait_for_next_task()
end

return WorkerTaskDispatcher
