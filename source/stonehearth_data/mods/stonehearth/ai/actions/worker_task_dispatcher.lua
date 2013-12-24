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
   if self._task then
      if self._started_action then
         self._task:notify_stopped_working()
         self._started_action = false
      end
      self._task = nil
   end
   
   if self._finish_fn then
      self._finish_fn(self._packed_action == nil)
      self._finish_fn = nil
   end
   self._packed_action = nil
   self._priority = 0

   local dispatch_fn = function(priority, packed_action, finish_fn, task)
      assert(not self._running)
      local abort_current, override
      if priority <= self._priority then
         log:debug('current action %s (pri %d) takes precedence over %s (pri %d)', 
                     self._packed_action[1], self._priority,
                     packed_action[1], priority)
         if finish_fn then
            finish_fn(false, packed_action)
         end
         return
      end
      if self._finish_fn and self._packed_action then
         log:debug('overriding current action %s (pri %d)', self._packed_action[1], self._priority)
         self._finish_fn(false, self._packed_action)
      end
      log:debug('preserving action %s (pri %d) for dispatch', packed_action[1], priority)                        
      self._packed_action = packed_action
      self._task = task
      self._finish_fn = finish_fn
      self._priority = priority
      self._ai:set_action_priority(self, priority)
   end   

   log:info('%s rejoining worker scheduler to wait for next action', self._entity)
   self._ai:set_action_priority(self, 0)
   self._scheduler:add_worker(self._entity, dispatch_fn)
end

function WorkerTaskDispatcher:run(ai, entity, ...)
   assert(self._packed_action, "worker dispatcher has no task to run")
   self._running = true
   if not self._task:notify_started_working() then
      log:info('%s rejecting action %s (too many workers!)', entity, self._packed_action[1])
      return
   end
   self._started_action = true
   
   log:info('%s executing action %s', entity, self._packed_action[1])

   --TODO: put this up over thier heads, like dialog!
   self._scheduler:remove_worker(self._entity)
   ai:execute(unpack(self._packed_action))
   self._packed_action = nil
end

function WorkerTaskDispatcher:stop()
   self._running = false
   self:_wait_for_next_task()
end

return WorkerTaskDispatcher
