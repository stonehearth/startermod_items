local TaskGroup = class()

function TaskGroup:__init(scheduler, activity)
   self._activity = activity
   self._scheduler = scheduler

   -- a table of all the workers which belong to the task group.   only
   -- workers in this group may run these tasks.  workers can be in
   -- any number of task groups
   self._workers = {}        

   -- a table of all the tasks in this group.  a task must be in exactly
   -- one task group.
   self._tasks = {}
end

function TaskGroup:join(worker)
   local id = worker:get_id()
   self._workers[id] = worker
end

function TaskGroup:leave(id)
   self._workers[id] = nil
end

function TaskGroup:create_task(name, args)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   local task = Task(self._scheduler, self, activity)
   self._tasks[task] = true;

   return task
end

function TaskGroup:recommend_worker_for(task, other_worker_priorities)
   assert(self._tasks[task], 'unowned task in _recommend_worker_for')
   local best_fitness, best_worker

   for id, worker in pairs(self._workers) do
      local fitness = self:_compute_worker_fitness(task, other_worker_priorities)
      if fitness and (not best_fitness or fitness < best_fitness) then
         best_fitness = fitness
         best_worker = worker
      end
   end
   return best_worker
end

function TaskGroup:_compute_worker_fitness(worker, task, other_worker_priorities)
   local task_pri = task:get_priority()
   local pri = other_worker_priorities[worker:get_id()]
   if pri and pri >= task_pri then
      -- this worker could never run if we fed it because it's currently doing something
      -- that has a higher priority.
      return nil
   end

   -- xxx: this is where we put in worker affinity....
   return task:_get_fitness(worker)
end

return TaskGroup
