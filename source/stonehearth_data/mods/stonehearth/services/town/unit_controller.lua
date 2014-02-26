local constants = require 'constants'
local UnitController = class()

function UnitController:__init(scheduler, entity)
   self._task_group = scheduler:create_task_group('stonehearth:unit_control', {})
                               :set_priority(stonehearth.constants.priorities.top.UNIT_CONTROL)   
                               :set_counter_name('unit_controller:' .. entity:get_id())
   self._task_group:add_worker(entity)
   self._scheduled_tasks = {}
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
