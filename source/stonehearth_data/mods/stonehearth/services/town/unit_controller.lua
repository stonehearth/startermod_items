local constants = require 'constants'
local UnitController = class()

function UnitController:__init(scheduler, entity)
   self._task_group = scheduler:create_task_group('stonehearth:unit_control', {})
                               :set_counter_name('unit_controller:' .. entity:get_id())
   self._task_group:add_worker(entity)
end

function UnitController:create_task(name, args)
   if self._current_task then
      self._current_task:destroy()
      self._current_task = nil
   end
   return self._task_group:create_task(name, args)
                              :set_priority(constants.priorities.unit_control.DEFAULT)
                              :once()
end

return UnitController
