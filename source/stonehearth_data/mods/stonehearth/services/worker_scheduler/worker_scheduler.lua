local WorkerTask = require 'services.worker_scheduler.worker_task'
local WorkerScheduler = class()

function WorkerScheduler:__init(faction)
   --radiant.log.info('constructing worker scheduler...')
   self._faction = faction
   self._workers = {}
   self._worker_tasks = {}
   radiant.events.listen('radiant.events.gameloop', self)
end

function WorkerScheduler:destroy()
   radiant.events.unlisten('radiant.events.gameloop', self)
end

WorkerScheduler['radiant.events.gameloop'] = function(self)
   --self:_check_build_orders()
   --self:_enable_pathfinders()
   --self:_dispatch_jobs()
end

function WorkerScheduler:_dispatch_solution(action, path)
   local id = path:get_source():get_id()
   local e = self._workers[id]

   assert(e, string.format('unknown worker id %d in _dispatch_solution', id))

   self:remove_worker(e.worker)
   e.dispatch_fn(10, action)
end

function WorkerScheduler:abort_worker_task(task)
   radiant.log.warning('aborting task in worker scheduler...')
end

function WorkerScheduler:add_worker_task(name)
   local task = WorkerTask(name, self)
   table.insert(self._worker_tasks, task)
   return task
end

function WorkerScheduler:add_worker(worker, dispatch_fn)
   assert(worker)
   local id = worker:get_id()

   radiant.log.info('adding worker %d.', id)
   -- xxx: figure out why we sometimes get multiple adds
   if not self._workers[id] then
      assert(self._workers[id] == nil)

      self._workers[id] = {
         worker = worker,
         dispatch_fn = dispatch_fn
      }
      self:_introduce_worker_to_tasks(worker)
   end
end

function WorkerScheduler:remove_worker(worker)
   assert(worker)

   local id = worker:get_id()
   radiant.log.info('removing worker %d.', id)
   self._workers[id] = nil
   self:_remove_worker_from_tasks(id)
end

function WorkerScheduler:_start_worker_task(task)
   assert(task.running, "logical error: call to _start_worker_task for non-running task")
   for id, e in pairs(self._workers) do
      task:_consider_worker(e.worker)
   end
end

function WorkerScheduler:_stop_worker_task(task)
   assert(not task.running, "logical error: call to _stop_worker_task for running task")
   for id, e in pairs(self._workers) do
      task:_remove_worker(e.worker)
   end
end

function WorkerScheduler:_introduce_worker_to_tasks(worker)
   for _, task in ipairs(self._worker_tasks) do
      if task.running then
         task:_consider_worker(worker)
      end
   end
end

function WorkerScheduler:_remove_worker_from_tasks(id)
   for _, task in ipairs(self._worker_tasks) do
      if task.running then
         task:_remove_worker(id)
      end
   end
end

return WorkerScheduler
