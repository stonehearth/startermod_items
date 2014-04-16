local Task = require 'services.server.tasks.task'
local TaskGroup = class()

local function rank_tasks_entries(l, r)
   if l.priority ~= r.priority then
      return l.priority > r.priority and l or r
   end
   return l.distance < r.distance and l or r
end

function TaskGroup:__init(scheduler, activity_name, args)
   args = args or {}
   self._activity = {
      name = activity_name,
      args = args
   }
   self._log = radiant.log.create_logger('task_group', activity_name)
   self._scheduler = scheduler
   self._counter_name = activity_name

   -- a table of all the workers which belong to the task group.   only
   -- workers in this group may run these tasks.  workers can be in
   -- any number of task groups
   self._workers = {}        

   -- a table of all the tasks in this group.  a task must be in exactly
   -- one task group.
   self._tasks = {}

   self._feeding = {}
   self._feeding_count = 0
end

function TaskGroup:set_counter_name(counter_name)
   self._counter_name = counter_name
   return self
end

function TaskGroup:get_activity()
   return self._activity
end

-- COMPLETELY untested.  DO NOT USE!! -- tonyc
function TaskGroup:destroy_all_tasks()
   assert(false)
   for task, _ in pairs(self._tasks) do
      task:destroy()
   end
   -- are they all gone?  
   assert(next(self._tasks) == nil)
end

function TaskGroup:add_worker(worker)
   local id = worker:get_id()
   assert(not self._workers[id])
   self._workers[id] = {
      worker = worker,
      running = nil,
      all_fed_tasks = {},
      all_fed_tasks_count = 0
   }
   return self
end

function TaskGroup:get_name()
   return self._activity.name
end

function TaskGroup:remove_worker(id)
   assert(type(id) == 'number')
   local entry = self._workers[id]
   if entry then
      for task, _ in pairs(entry.all_fed_tasks) do
         task:_unfeed_worker(id)
      end
      self._workers[id] = nil
   end
   return self
end

function TaskGroup:create_task(activity_name, args)
   assert(type(activity_name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = activity_name,
      args = args or {}
   }
   
   local task = Task(self, activity)
   self._tasks[task] = task

   self._log:debug('created task %s', task:get_name())   

   return task
end

function TaskGroup:_on_task_destroy(task)
   self._log:debug('removing task %s from group', task:get_name())
   if self._tasks[task] then
      self:_stop_feeding_task(task)
      self:_unfeed_workers_from_task(task)      
      self._tasks[task] = nil
   end
end

function TaskGroup:_unfeed_workers_from_task(task)
   self._log:debug('removing workers from task %s', task:get_name())

   for worker_id, entry in pairs(self._workers) do
      if entry.running == task then
         self._log:debug('clearing %s running task %s', tostring(entry.worker), task:get_name())
         entry.running = nil
         task:_unfeed_worker(entry.worker)
         assert(entry.all_fed_tasks[task])
      end
      if entry.all_fed_tasks[task] then
         self._log:debug('clearing %s available task %s', tostring(entry.worker), task:get_name())
         task:_unfeed_worker(entry.worker)
         entry.all_fed_tasks[task] = nil
         entry.all_fed_tasks_count = entry.all_fed_tasks_count - 1
      end
   end
end

-- called by the task to notify the group that it's ready to receive
-- workers.
function TaskGroup:_start_feeding_task(task)
   self._log:debug('starting to feed task %s', task:get_name())
   assert (not self._feeding[task])
   self._feeding[task] = task
   self._feeding_count = self._feeding_count + 1
   self:_set_performance_counter('feeding_task_count', self._feeding_count)
end

-- called by the task to ask the group not to send any more workers.  the
-- task will call _start_feeding_task() when it's ready for more.
function TaskGroup:_stop_feeding_task(task)
   self._log:debug('stopping feeding of task %s', task:get_name())
   if self._feeding[task] then
      self._feeding[task] = nil      
      self._feeding_count = self._feeding_count - 1
      self:_set_performance_counter('feeding_task_count', self._feeding_count)
   end
end

function TaskGroup:_notify_worker_started_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry then
      assert(not entry.running)
      assert(entry.all_fed_tasks[task])

      self._log:debug('marking %s running task %s', tostring(worker), task:get_name())
      entry.running = task
   end
end

function TaskGroup:_notify_worker_stopped_task(task, worker)
   local worker_id = worker:get_id()
   local entry = self._workers[worker_id]
   if entry and task == entry.running then
      self._log:debug('marking %s as stopping task %s', tostring(worker), task:get_name())
      entry.running = nil
   end
end

function TaskGroup:_prioritize_tasks_for_worker(entry)
   local tasks = {}

   for task, _ in pairs(self._feeding) do
      local priority, distance = self:_get_task_fitness(task, entry)
      if priority then
         local matching_entry = {
            task = task,
            priority = priority,
            distance = distance,
         }
         table.insert(tasks, matching_entry)
      end
   end
   table.sort(tasks, function (l, r)      
         -- sort places entries in 'ascending' order.  for our purposes, the item at
         -- the front of the list should be the one with the best ranking.  therefore,
         -- 'l < r' equates to 'is l of higher priority than r'.
         return rank_tasks_entries(l, r) == l
      end)
   return tasks
end

function TaskGroup:_get_task_fitness(task, entry)
   -- if the worker is already doing something higher priority than this
   -- task, there's no point in trying to feed it

   -- if entry.running and entry.running:get_priority() <= task:get_priority() then
   -- assert(not entry.running)
   
   -- if the worker is already feeding in this task, don't feed again!
   if entry.all_fed_tasks[task] then
      self._log:detail('skipping fitness check on task %s for %s (already feeding)',
                       task:get_name(), tostring(entry.worker))
      return nil
   end
   local priority, distance = task:_get_fitness(entry.worker)
   self._log:detail('task %s fitness for %s priority:%d distance:%.2f',
                    task:get_name(), tostring(entry.worker), 
                    priority, distance)
   return priority, distance
end

function TaskGroup:_prioritize_worker_queue()
   local workers = {}

   -- workers with the lowest fitness value get to go first
   for id, entry in pairs(self._workers) do
      if not entry.worker:is_valid() then
         self:remove_worker(id)
      else--if not entry.running then
         local matching_entry = {
            worker = entry.worker,
            all_fed_tasks_count = entry.all_fed_tasks_count,
            task_rankings = self:_prioritize_tasks_for_worker(entry)
         }
         table.insert(workers, matching_entry)
      end
   end
   self:_set_performance_counter('worker_queue_len', #workers)

   table.sort(workers, function (l, r)
         return l.all_fed_tasks_count < r.all_fed_tasks_count
      end)
   
   if self._log:is_enabled(radiant.log.DETAIL) then
      for i, entry in ipairs(workers) do
         self._log:detail('%2d previously fed for %s', i, entry.all_fed_tasks_count, tostring(entry.worker))
         self._log:detail('%2d tasks ready for feeding:', #entry.task_rankings)
         for j, task_entry in ipairs(entry.task_rankings) do
            self._log:detail('   %d) : %10s (pri:%d d:%4.2f)', j, task_entry.task:get_name(), task_entry.priority, task_entry.distance)
         end
      end
   end

   return workers
end

function TaskGroup:_propose(worker_entry, task_entry, engagements)
   -- if the task is free, just keep it
   local current = engagements[task_entry.task]
   if not current then
      engagements[task_entry.task] =  {
         proposed_worker_entry = worker_entry,
         priority = task_entry.priority,
         distance = task_entry.distance,
      }
      return true
   end
   -- if the task isn't better than the current one, keep the engagement
   if rank_tasks_entries(task_entry, current) == current then
      return true
   end

   -- break it off
   local propose_again_entry = {
      task = task_entry.task,
      priority = current.priority,
      distance = current.distance,
   }
   -- notify the current worker that the engagement is broken
   table.insert(current.proposed_worker_entry.task_rankings, propose_again_entry)
   table.sort(current.proposed_worker_entry.task_rankings, function (l, r)
         return rank_tasks_entries(l, r) == l
      end)

   -- update the engagement   
   current.proposed_worker_entry = worker_entry
   current.priority = task_entry.priority
   current.distance = task_entry.distance
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
      self._log:debug('feeding worker %s to task %s', tostring(worker), task:get_name())
      local entry = self._workers[worker:get_id()]

      assert(not entry.all_fed_tasks[task])
      entry.all_fed_tasks[task] = true
      entry.all_fed_tasks_count = entry.all_fed_tasks_count + 1
      task:_feed_worker(entry.worker)
   end
   
   return total_proposals
end

function TaskGroup:get_counter_name()
   return self._scheduler:get_counter_name() .. ':task_group:' .. self._counter_name
end

function TaskGroup:_set_performance_counter(name, value)
   radiant.set_performance_counter(self:get_counter_name() .. ':' .. name, value)
end

return TaskGroup
