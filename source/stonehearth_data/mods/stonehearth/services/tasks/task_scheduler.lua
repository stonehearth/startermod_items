local PriorityQueue = require 'services.tasks.priority_queue'
local TaskScheduler = class()

local get_task_priority(task)
   return task:get_priority()
end

function TaskScheduler:__init()
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

function TaskScheduler:create_task_group(activity)
   return TaskGroup(self, activity)
end

function TaskScheduler:_start_worker_activity(worker, activity, priority)
   local activities = self:_get_running_activity_list(activity)
   activities[worker:get_id()] = priority
end

function TaskScheduler:_start_feeding_task(task)
   assert(task:is_running())

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
   local activity = task:get_activity()

   local current_activities = self:_get_running_activity_list(activity)
   if current_activities[worker_id] then
      self._log:warning('duplicate worker for %s in running activities.', tostring(worker))
   end
   current_activities[worker_id] = task:get_priority()
end

function TaskScheduler:_notify_worker_stopped_task(task, worker)
   local worker_id = worker:get_id()
   local activity = task:get_activity()

   local current_activities = self:_get_running_activity_list(activity)
   if not current_activities[worker_id] then
      self._log:warning('unknown worker for %s in running activities.', tostring(worker))
   end
   current_activities[worker_id] = nil
end

function TaskScheduler:_update()
   self._process_feeding_queue()
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
   local activity = task:get_activity()
   local task_group = task_get_task_group()
   local current_activities = self:_get_running_activity_list(activity)
   local worker = task_group:recommend_worker_for(task, current_activities)

   if not worker then
      self._log:detail('no worker available for %s', task:get_name())
      return
   end
   task:_add_worker(worker)
   current_activities[activity][worker_id] = task:get_priority()
   self._feed_count = self._feed_count + 1
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

