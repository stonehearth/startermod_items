local TaskGroup = require 'services.tasks.task_group'
local TaskScheduler = class()

function TaskScheduler:__init(name)
   self._name = name
   self._counter_name = name
   self._log = radiant.log.create_logger('tasks.scheduler', 'scheduler: ' .. self._name)

   self._task_groups = {}
   self._poll_interval = 200
   self._max_feed_per_interval = 1

   self:_start_update_timer()
end

function TaskScheduler:get_name()
   return self._name
end

function TaskScheduler:get_counter_name()
   return 'task_scheduler:' .. self._counter_name
end

function TaskScheduler:set_counter_name(counter_name)
   self._counter_name = counter_name
   return self
end


function TaskScheduler:create_task_group(activity_name, args)
   local task_group = TaskGroup(self, activity_name, args)
   table.insert(self._task_groups, task_group)
   return task_group
end

function TaskScheduler:_update()
   local feed_count = self._max_feed_per_interval
   for i, task_group in ipairs(self._task_groups) do
      local count = task_group:_update(feed_count)
      feed_count = feed_count - count
      if feed_count <= 0 then
         break
      end
   end
   self:_start_update_timer()
end

function TaskScheduler:_start_update_timer()
   radiant.set_realtime_timer(self._poll_interval, function()
         self:_update()
      end)
end

function TaskScheduler:_set_performance_counter(name, value)
   radiant.set_performance_counter(self:get_counter_name() .. ':' .. name, value)
end

return TaskScheduler

