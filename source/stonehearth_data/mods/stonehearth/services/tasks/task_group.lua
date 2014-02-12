local Task = require 'services.tasks.task'
local TaskGroup = class()

function TaskGroup:__init(scheduler, name, args)
   args = args or {}
   self._activity = {
      name = name,
      args = args
   }
   self._log = radiant.log.create_logger('task_group', name)
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

   self._log:debug('created task [%d:%s]', task:get_id(), task:get_name())

   return task
end

function TaskGroup:_destroy_task(task)
   self._log:debug('removing task [%d:%s] from group', task:get_id(), task:get_name())
   if self._tasks[task] then
      local task_id = task:get_id()
      self._tasks[task] = nil
      self._feeding[task] = nil
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
   self._log:debug('starting to feed task [%d:%s]', task:get_id(), task:get_name())
   assert (not self._feeding[task])
   self._feeding[task] = task
end

function TaskGroup:_stop_feeding_task(task)
   self._log:debug('stopping feeding of task [%d:%s]', task:get_id(), task:get_name())
   if self._feeding[task] then
      self._feeding[task] = nil      
   end
end

function TaskGroup:_notify_worker_started_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry then
      assert(not entry.running)
      self._log:debug('marking %s running task [%d:%s]', tostring(worker), task:get_id(), task:get_name())
      entry.running = task
      -- stop feeding in everyone else
      for feeding_task, _ in pairs(entry.feeding) do
         if task ~= feeding_task then
            entry.feeding[feeding_task] = nil
            self._log:detail('removing feeding task [%d:%s] from %s', feeding_task:get_id(), feeding_task:get_name(), tostring(worker))
            feeding_task:_remove_worker(worker_id)
         end
      end
   end
end

function TaskGroup:_notify_worker_stopped_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry and task == entry.running then
      self._log:debug('clearing %s running task (was: %s)', tostring(worker), task:get_name())
      entry.running = nil
      entry.feeding[task] = nil
      -- remove it so we can re-prioritize tasks...
      --  task:_remove_worker(worker_id)
   end
end

function TaskGroup:_prioritize_tasks_for_worker(entry)
   local tasks = {}

   for task, _ in pairs(self._feeding) do
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

   -- if entry.running and entry.running:get_priority() <= task:get_priority() then
   assert(not entry.running)
   
   -- if the worker is already feeding in this task, don't feed again!
   if entry.feeding[task] then
      self._log:detail('skipping fitness check on task [%d:%s] for %s (already feeding)',
                       task:get_id(), task:get_name(), tostring(entry.worker))
      return nil
   end
   local priority, distance = task:_get_fitness(entry.worker)
   self._log:detail('task [%d:%s] fitness for %s priority:%d distance:%.2f',
                    task:get_id(), task:get_name(), tostring(entry.worker), 
                    priority, distance)
   local fitness = -priority * 20 + distance
   return fitness
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
   
   if self._log:is_enabled(radiant.log.DETAIL) then
      for i, entry in ipairs(workers) do
         self._log:detail('#%d : %4.2f -> %s', i, entry.worker_fitness, tostring(entry.worker))
         for j, task_entry in ipairs(entry.task_rankings) do
            self._log:detail('   %d) : %4.2f -> [%d:%s]', j, task_entry.task_fitness, task_entry.task:get_id(), task_entry.task:get_name())
         end
      end
   end

   return workers
end

function TaskGroup:_get_worker_fitness(entry)
   if entry.running then
      return nil
   end
   if #entry.feeding > 0 then
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
      local worker = engagement.proposed_worker_entry.worker
      self._log:debug('feeding worker %s to task [%d:%s]', tostring(worker), task:get_id(), tostring(task:get_name()))
      local entry = self._workers[worker:get_id()]

      assert(not entry.feeding[task])
      entry.feeding[task] = engagement.proposed_task_fitness
      task:_add_worker(entry.worker)
   end
   
   return total_proposals
end
return TaskGroup
