local WorkerTaskDispatcher = class()
local log = radiant.log.create_logger('worker')

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
   if self._finish_fn then
      self._finish_fn(self._task == nil)
      self._finish_fn = nil
   end
   self._task = nil

   local dispatch_fn = function(priority, packed_action, finish_fn)
      self._task = packed_action
      self._finish_fn = finish_fn
      self._ai:set_action_priority(self, priority)
   end   

   log:info('%s rejoining worker scheduler to wait for next action', self._entity)
   self._ai:set_action_priority(self, 0)
   self._scheduler:add_worker(self._entity, dispatch_fn)
end

function WorkerTaskDispatcher:run(ai, entity, ...)
   assert(self._task, "worker dispatcher has no task to run")
   log:info('%s executing action %s', entity, self._task[1])

   --TODO: put this up over thier heads, like dialog!
   self._scheduler:remove_worker(self._entity)
   ai:execute(unpack(self._task))
   self._task = nil
end

function WorkerTaskDispatcher:stop()
   self:_wait_for_next_task()
end

return WorkerTaskDispatcher
