local Task = require 'services.tasks.task'
local TaskGroup = class()

function TaskGroup:__init(scheduler, name, args)
   args = args or {}
   self._activity = {
      name = name,
      args = args
   }
   self._scheduler = scheduler

   -- a table of all the workers which belong to the task group.   only
   -- workers in this group may run these tasks.  workers can be in
   -- any number of task groups
   self._workers = {}        

   -- a table of all the tasks in this group.  a task must be in exactly
   -- one task group.
   self._tasks = {}

   self._feeding = {}
end

function TaskGroup:get_activity()
   return self._activity
end

function TaskGroup:add_worker(worker)
   local id = worker:get_id()
   assert(not self._workers[id])
   self._workers[id] = {
      worker = worker,
      running = nil,
      feeding = {},
   }
end

function TaskGroup:get_name()
   return self._activity.name
end

function TaskGroup:remove_worker(id)
   local entry = self._workers[id]
   if entry then
      for task, _ in entry.feeding do
         task:_remove_worker(id)
      end
      self._workers[id] = nil
   end
end

function TaskGroup:create_task(name, args)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   local task = Task(self, activity)
   self._tasks[task] = task

   return task
end

function TaskGroup:_destroy_task(task)
   if self._tasks[task] then
      self._tasks[task] = nil
      self:_remove_workers_from_task(task)      
   end
end

function TaskGroup:_remove_workers_from_task(task)
   for worker_id, entry in pairs(self._workers) do
      if entry.feeding[task] then
         task:_remove_worker(entry.worker)
         entry.feeding[task] = nil
      end
   end
end

function TaskGroup:_start_feeding_task(task)
   local id = task:get_id()
   assert (not self._feeding[id])
   self._feeding[id] = task
end

function TaskGroup:_stop_feeding_task(task)
   local id = task:get_id()
   if self._feeding[id] then
      self._feeding[id] = nil      
   end
end

function TaskGroup:_notify_worker_started_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry then
      assert(not entry.running)
      entry.running = task
   end
end

function TaskGroup:_notify_worker_stopped_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry then
      assert(entry.running)
      entry.running = nil
   end
end


function TaskGroup:_prioritize_tasks_for_worker(entry)
   local tasks = {}

   for id, task in pairs(self._feeding) do
      local task_fitness = self:_get_task_fitness(task, entry)
      if task_fitness then
         local matching_entry = {
            task = task,
            task_fitness = task_fitness,
         }
         table.insert(tasks, matching_entry)
      end
   end
   table.sort(tasks, function (l, r)
         return l.task_fitness < r.task_fitness
      end)
   return tasks
end

function TaskGroup:_get_task_fitness(task, entry)
   -- if the worker is already doing something higher priority than this
   -- task, there's no point in trying to feed it
   if entry.running and entry.running:get_priority() <= task:get_priority() then
      return nil
   end
   -- if the worker is already feeding in this task, don't feed again!
   if entry.feeding[task] then
      return nil
   end
   return task:_get_fitness(entry.worker)
end

function TaskGroup:_prioritize_worker_queue()
   local workers = {}

   -- workers with the lowest fitness value get to go first
   for id, entry in pairs(self._workers) do
      local worker_fitness = self:_get_worker_fitness(entry)
      if worker_fitness then
         local matching_entry = {
            worker = entry.worker,
            worker_fitness = worker_fitness,
            task_rankings = self:_prioritize_tasks_for_worker(entry)
         }
         table.insert(workers, matching_entry)
      end
   end

   table.sort(workers, function (l, r)
         return l.worker_fitness < r.worker_fitness
      end)

   return workers
end

function TaskGroup:_get_worker_fitness(entry)
   if entry.running then
      return nil
   end
   return #entry.feeding
end

function TaskGroup:_propose(worker_entry, task_entry, engagements)
   -- if the task is free, just keep it
   local current = engagements[task_entry.task]
   if not current then
      engagements[task_entry.task] =  {
         proposed_worker_entry = worker_entry,
         proposed_task_fitness = task_entry.task_fitness
      }
      return true
   end
   -- if the task isn't better than the current one, keep the engagement
   if task_entry.task_fitness >= current.proposed_task_fitness then
      return true
   end

   -- break it off
   local propose_again_entry = {
      task = task_entry.task,
      task_fitness = current.proposed_task_fitness,
   }
   -- notify the current worker that the engagement is broken
   table.insert(current.proposed_worker_entry.task_rankings, propose_again_entry)
   table.sort(current.proposed_worker_entry.task_rankings, function (l, r)
         return l.task_fitness < r.task_fitness
      end)
   -- update the engagement   
   current.proposed_worker_entry = worker_entry
   current.proposed_task_fitness = task_entry.task_fitness
   return false
end

function TaskGroup:_iterate(workers, engagements, max_proposals)
   local proposal_count = 0
   for _, worker_entry in ipairs(workers) do
      while #worker_entry.task_rankings > 0 do
         local task_entry = table.remove(worker_entry.task_rankings, 1)
         if not self:_propose(worker_entry, task_entry, engagements) then
            return proposal_count, false
         else
            proposal_count = proposal_count + 1
            if proposal_count >= max_proposals then
               return proposal_count, true
            end
         end
      end
   end
   return proposal_count, true
end

function TaskGroup:_update(count)
   local finished
   local engagements = {}
   local workers = self:_prioritize_worker_queue()
   local proposal_count, total_proposals = 0, 0
   repeat
      proposal_count, finished = self:_iterate(workers, engagements, count - total_proposals)
      total_proposals = total_proposals + proposal_count
      assert(total_proposals <= count)
   until finished or total_proposals == count

   for task, engagement in pairs(engagements) do
      local worker_id = engagement.proposed_worker_entry.worker:get_id()
      local entry = self._workers[worker_id]
      self:_add_worker_to_task(task, entry)
   end
   
   return total_proposals
end

function TaskGroup:_add_worker_to_task(task, entry)
   assert(not entry.feeding[task])
   entry.feeding[task] = true
   task:_add_worker(entry.worker)
end

return TaskGroup
