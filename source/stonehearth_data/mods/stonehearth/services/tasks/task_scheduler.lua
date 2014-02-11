local TaskGroup = require 'services.tasks.task_group'
local TaskScheduler = class()

function get_task_priority(task)
   return task:get_priority()
end

function TaskScheduler:__init(name)
   self._name = name
   self._log = radiant.log.create_logger('tasks.scheduler', 'scheduler: ' .. self._name)

   self._task_groups = {}
   self._poll_interval = 200
   self._max_feed_per_interval = 1

   self:_start_update_timer()
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
   radiant.set_timer(radiant.gamestate.now() + self._poll_interval, function()
         self:_update()
      end)
end

return TaskScheduler

