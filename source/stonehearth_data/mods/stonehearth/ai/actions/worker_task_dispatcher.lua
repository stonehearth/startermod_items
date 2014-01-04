local WorkerTaskDispatcher = class()
local log = radiant.log.create_logger('worker')

WorkerTaskDispatcher.name = 'worker task dispatcher'
WorkerTaskDispatcher.does = 'stonehearth:top'
WorkerTaskDispatcher.version = 1
WorkerTaskDispatcher.priority = 0


function WorkerTaskDispatcher:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   self._queued = {}
   
   local faction = self._entity:get_component('unit_info'):get_faction()
   
   self._scheduler = stonehearth.worker_scheduler:get_worker_scheduler(faction)   
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

   local dispatch_fn = function(priority, packed_action, finish_fn, task, distance)
      assert(not self._running)
      local entry = {
         priority = priority,
         packed_action = packed_action,
         distance = distance,
         finish_fn = finish_fn,
         task = task
      }
      -- add the entry to our queue action table.  then sort by priority and
      -- set our to the highest priority entry.  break dies on equal priority
      -- tasks by preferring the one that's closest
      table.insert(self._queued, entry)
      table.sort(self._queued, function(a, b) 
            if a.priority == b.priority then
               return a.distance > b.distance
            end
            return a.priority < b.priority 
         end)
      self._ai:set_action_priority(self, self._queued[#self._queued].priority)
   end   

   log:info('%s rejoining worker scheduler to wait for next action', self._entity)
   self._ai:set_action_priority(self, 0)
   if not self._in_worker_scheduler then
      self._scheduler:add_worker(self._entity, dispatch_fn)
   end
end

function WorkerTaskDispatcher:run(ai, entity, ...)
   assert(not self._task, "stale task leftover from last call to run")
   assert(#self._queued > 0, "worker dispatcher has no task to run")
   
   self._running = true
   self._scheduler:remove_worker(self._entity)
   
   -- find something to do.  entries are sorted in priority order (highest at
   -- the back).  keep trying to do something until we find a task which will
   -- let us proceed.
   while #self._queued > 0 do
      local entry = table.remove(self._queued)
      if entry.task:notify_started_working() then
         -- yay!  this one's good.  remember some state and break out of the loop
         self._task = entry.task
         self._packed_action = entry.packed_action
         self._finish_fn = entry.finish_fn
         self._started_action = true         
         break;
      end
      -- boo!  this one's no good.  call the finish function (if it's there) and
      -- keep searching.
      log:info('%s rejecting action %s (too many workers!)', entity, entry.packed_action[1])
      if entry.finish_fn then
         entry.finish_fn(false)
      end
   end
   
   -- now call the finish function on all the tasks we rejected...
   while #self._queued > 0 do
      local entry = table.remove(self._queued)
      log:info('%s rejecting action %s (priority too low)', entity, entry.packed_action[1])
      if entry.finish_fn then
         entry.finish_fn(false)
      end
   end
   
   -- ...and execute the action
   if self._packed_action then
      log:info('%s executing action %s', entity, self._packed_action[1])

      --TODO: put this up over thier heads, like dialog!
      ai:execute(unpack(self._packed_action))
      self._packed_action = nil
   end
end

function WorkerTaskDispatcher:stop()
   self._running = false
   self:_wait_for_next_task()
end

return WorkerTaskDispatcher
