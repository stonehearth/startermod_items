local constants = require 'constants'
local UnitController = class()

function UnitController:__init(scheduler, entity, activity_name, priority)
   self._task_group = scheduler:create_task_group(activity_name, {})
                               :set_priority(priority)   
                               :set_counter_name('unit_controller:' .. entity:get_id())
   self._task_group:add_worker(entity)
   self._scheduled_tasks = {}
end

function UnitController:destroy()
end

function UnitController:create_immediate_task(name, args)
   if self._immediate_task then
      self._immediate_task:destroy()
      self._immediate_task = nil
   end

   if #self._scheduled_tasks > 0 then
      for _, task in ipairs(self._scheduled_tasks) do
         task:destroy()
      end
      self._scheduled_tasks = {}
   end

   self._immediate_task = self:_create_task(name, args)
   return self._immediate_task
end

-- A task that the dude will try to do when he's not given an immediate task by the player
function UnitController:create_scheduled_task(name, args)
   local task = self:_create_task(name, args)
   table.insert(self._scheduled_tasks, task)
   return task
end

function UnitController:_create_task(name, args)
   return self._task_group:create_task(name, args)
                          :set_priority(constants.priorities.unit_control.DEFAULT)
end

return UnitController
