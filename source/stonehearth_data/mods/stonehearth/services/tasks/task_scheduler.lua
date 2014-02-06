local TaskQueue = require 'services.tasks.task_queue'
local TaskScheduler = class()

function TaskScheduler:__init()
   self._feeding = TaskQueue('feeding')
   self._pending = TaskQueue('pending')
end

function TaskScheduler:_add_task(task)
   local id = task:get_id()
   self._idle[id] = task
end

function TaskScheduler:_start_feeding_task(task)
   assert(task:is_running())

   local id = task:get_id()

   self._pending:remove(id)
   self._feeding:add(id, task)

   self:_feed_tasks()
end

function TaskScheduler:_feed_tasks()
   while self._feeding:count() < self._max_feed_count do
      local task = self._pending:get_highest_priority_task()
   end
end

function TaskScheduler:_get_task_priority(task)
   local priority = task:get_priority()
   return priority
end

return TaskScheduler

