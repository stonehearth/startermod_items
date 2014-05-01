local TaskGroup = require 'services.server.tasks.task_group'
local TaskScheduler = class()

function TaskScheduler:__init(name)
   self._name = name
   self._counter_name = name
   self._log = radiant.log.create_logger('task_scheduler', 'scheduler: ' .. self._name)
   self._log:debug('created task scheduler')

   self._task_groups = {}
   self._poll_interval = radiant.util.get_config('task_poll_interval', 100)
   self._max_feed_per_interval = radiant.util.get_config('task_max_feed_per_interval', 5)
   self._last_task_group_index = 1

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

--- Update task groups with queued tasks
--  Iterate through the task groups, starting at the saved index
--  Feed max_feed_per_interval tasks from the task groups and then stop
--  Also, stop if we've iterated through all the task groups and there are no more tasks to feed
--  Save the last checked task group so we can continue from there next time
function TaskScheduler:_update()
   self._log:debug('updating task scheduler (%d groups)', #self._task_groups)
   
   local num_to_feed = self._max_feed_per_interval
   local num_groups_evaluated = 0

   while num_to_feed > 0 and 
         num_groups_evaluated < #self._task_groups and 
         #self._task_groups > 0 do

      if self._last_task_group_index > #self._task_groups then
         self._last_task_group_index = 1
      end

      local target_task_group = self._task_groups[self._last_task_group_index]
      
      --Increment the target task group
      self._last_task_group_index = self._last_task_group_index + 1
      
      local count = target_task_group:_update(num_to_feed)
      num_to_feed = num_to_feed - count

      num_groups_evaluated = num_groups_evaluated + 1
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

