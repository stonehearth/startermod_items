local PriorityQueue = require 'services.tasks.priority_queue'
local TaskGroup = require 'services.tasks.task_group'
local TaskScheduler = class()

function get_task_priority(task)
   return task:get_priority()
end

function TaskScheduler:__init(name)
   self._name = name
   self._log = radiant.log.create_logger('tasks.scheduler', 'scheduler: ' .. self._name)
   self._running_activities = {}

   -- how many tasks have we fed since we reset the feed count?
   self._feed_count = 0

   -- the feeding queue contains all the task which are still looking for entities
   -- to inject their actions into.  the pending queue contains all tasks which would
   -- like to start feeding, but we have queues because our parallelism threshold has
   -- been reached.  tasks which are fully feeded and running are not tracked by
   -- the scheduler at all.
   self._feeding = PriorityQueue('feeding', get_task_priority)
   --self._pending = PriorityQueue('pending', get_task_priority)
end

function TaskScheduler:get_name()
   return self._name
end

function TaskScheduler:create_task_group(name, args)
   return TaskGroup(self, name, args)
end

function TaskScheduler:_start_feeding_task(task)
   local id = task:get_id()
   
   --self._pending:remove(id)
   self._feeding:add(id, task)
   self:_update()
end

function TaskScheduler:_stop_feeding_task(task)
   local id = task:get_id()
   self._feeding:remove(id)
end

function TaskScheduler:_notify_worker_started_task(task, worker)
   local worker_id = worker:get_id()
   local current_activities = self:_get_running_activity_list_for_task(task)
   if current_activities[worker_id] then
      self._log:warning('duplicate worker for %s in running activities.', tostring(worker))
   end
   current_activities[worker_id] = task:get_priority()
end

function TaskScheduler:_notify_worker_stopped_task(task, worker)
   local worker_id = worker:get_id()
   local current_activities = self:_get_running_activity_list_for_task(task)
   if not current_activities[worker_id] then
      self._log:warning('unknown worker for %s in running activities.', tostring(worker))
   end
   current_activities[worker_id] = nil
end

function TaskScheduler:_update()
   self:_process_feeding_queue()
end

function TaskScheduler:_process_feeding_queue()
   -- keep feeding tasks which need to be fed
   for task in self._feeding:round_robin() do
      self:_feed_task(task)
   end
end

function TaskScheduler:_feed_task(task)
   -- ask the task group for a worker to feed the task with.  this is genearlly the
   -- worker with the highest priority to do the task that is not already doing something
   -- else at higher priority that falls in the same activity bucket.  for example, if
   -- a worker is already building scaffolding, there's no sense at all in recommending
   -- him to the restock-stockpile task!  only we know the mapping of entities to activities,
   -- so pass that into the task group
   local task_group = task:get_task_group()
   local current_activities = self:_get_running_activity_list_for_task(task)
   local worker = task_group:recommend_worker_for(task, current_activities)

   if not worker then
      self._log:detail('no worker available for %s', task:get_name())
      return
   end
   task:_add_worker(worker)
   self._feed_count = self._feed_count + 1
end

function TaskScheduler:_get_running_activity_list_for_task(task)
   local activity = task:get_task_group():get_activity().name
   return self:_get_running_activity_list(activity)   
end

function TaskScheduler:_get_running_activity_list(activity)
   local activities = self._running_activities[activity]
   if not activities then
      activities = {}
      self._running_activities[activity] = activities
   end
   return activities
end

return TaskScheduler

