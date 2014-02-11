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

   --self._pending = PriorityQueue('pending', get_task_priority)
   self._task_groups = {}
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self._update)
end

function TaskScheduler:get_name()
   return self._name
end

function TaskScheduler:create_task_group(name, args)
   local task_group = TaskGroup(self, name, args)
   table.insert(self._task_groups, task_group)
   return task_group
end

function TaskScheduler:_update()
   for i, task_group in ipairs(self._task_groups) do
      task_group:_update()
   end
end

return TaskScheduler

