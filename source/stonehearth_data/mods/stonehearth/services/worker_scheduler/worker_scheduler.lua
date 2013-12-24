local priorities = require('constants').priorities.worker_task
local WorkerTask = require 'services.worker_scheduler.worker_task'
local WorkerDispatcher = require 'services.worker_scheduler.worker_dispatcher'
local WorkerScheduler = class()
local log = radiant.log.create_logger('worker.scheduler')

function WorkerScheduler:__init(faction)
   log:debug('constructing worker scheduler...')
   self._faction = faction
   self._tasks = {}
   self._dispatchers = {}
   self._strict = true
end

function WorkerScheduler:dispatch_solution(priority, worker_id, destination_id, action, finish_fn, task, distance)
   assert(finish_fn)
   local dispatcher = self._dispatchers[worker_id]
   
   if self._strict then
      --TODO: This is getting really frustrating.
      --I don't know why the workers are being cycled through twice, but it 
      --happens 100% of the time we try to make 2 stockpiles with exclusive requirements.
      --When not strict, game proceeds fine
      assert(dispatcher, string.format('unknown worker id %d in _dispatch_solution', worker_id))
   elseif not dispatcher then
      log:warning('unknown worker id %d in _dispatch_solution', worker_id)
      finish_fn(false)
      return
   end
   -- TODO: let's see how this works out.  completely bypass the WorkerDispatcher and invoke
   -- the callback immediately.  This makes the workers more responsive, but they might make
   -- very sub-optimal decisions
   dispatcher._dispatch_fn(priority, action, finish_fn, task, distance)
   --dispatcher:add_solution(destination_id, priority, action, finish_fn, task)
end

function WorkerScheduler:add_worker_task(name)
   local task = WorkerTask(name, self)
   table.insert(self._tasks, task)
   log:info('%s added to worker scheduler', task:get_name())
   return task
end

function WorkerScheduler:remove_worker_task(task)
   for i, t in ipairs(self._tasks) do
      if t == task then
         log:info('%s removed from worker scheduler', tostring(task))
         table.remove(self._tasks, i)
         break
      end
   end
end

function WorkerScheduler:add_worker(worker, dispatch_fn)
   assert(worker)
   local id = worker:get_id()
   local dispatcher = self._dispatchers[id]
   
   if self._strict then
      assert(not dispatcher, 'adding duplicate worker %s to worker scheduler', worker)
   elseif dispatcher then
      log:warning('adding duplicate worker %s to worker scheduler', worker)
      return
   end
   
   log:info('%s adding to worker scheduler', worker)
   self._dispatchers[id] = WorkerDispatcher(worker, dispatch_fn)
   self:_introduce_worker_to_tasks(worker)
end

function WorkerScheduler:remove_worker(worker)
   assert(worker)
   log:info('%s removing from worker scheduler', worker)

   local id = worker:get_id()
   local dispatcher = self._dispatchers[id]
   if dispatcher then
      dispatcher:destroy()
      self._dispatchers[id] = nil
   end
   self:_remove_worker_from_tasks(id)
end

function WorkerScheduler:_start_worker_task(task)
   assert(task:is_running(), "logical error: call to _start_worker_task for non-running task")
   for id, d in pairs(self._dispatchers) do
      task:_consider_worker(d:get_worker())
   end
end

function WorkerScheduler:_stop_worker_task(task)
   assert(not task:is_running(), "logical error: call to _stop_worker_task for running task")
   for id, d in pairs(self._dispatchers) do
      task:_remove_worker(id)
   end
end

function WorkerScheduler:_introduce_worker_to_tasks(worker)
   for _, task in ipairs(self._tasks) do
      if task:is_running() then
         task:_consider_worker(worker)
      end
   end
end

function WorkerScheduler:_remove_worker_from_tasks(id)
   for _, task in ipairs(self._tasks) do
      if task:is_running() then
         task:_remove_worker(id)
      end
   end
end

return WorkerScheduler
