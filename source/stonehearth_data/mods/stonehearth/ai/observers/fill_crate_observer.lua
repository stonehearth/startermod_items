local log = radiant.log.create_logger('fill_crate_observer')

local FillCrateObserver = class()

function FillCrateObserver:initialize(entity)
   self._sv.entity = entity
end

function FillCrateObserver:activate()
   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)

   if player_id ~= '' then
      self:_create_worker_task()
   else
      self._unit_info_trace = self._sv.entity:add_component('unit_info'):trace_player_id('filter observer')
         :on_changed(function()
               self._unit_info_trace:destroy()
               self._unit_info_trace = nil
               self:_create_worker_task()
            end)

   end
end

function FillCrateObserver:_create_worker_task()
   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)
   local town = stonehearth.town:get_town(player_id)
   if town then 
      log:error('creating fill_crate task')
      self._fill_crate_task = town:create_task_for_group('stonehearth:task_group:restock', 'stonehearth:fill_crate', { crate = self._sv.entity })
         :set_source(self._sv.entity)
         :set_name('fill crate task')
         :set_priority(stonehearth.constants.priorities.simple_labor.RESTOCK_STOCKPILE)
      self._fill_crate_task:start()
   end
end


function FillCrateObserver:destroy()
   if self._unit_info_trace then
      self._unit_info_trace:detroy()
      self._unit_info_trace = nil
   end

   if self._fill_crate_task then
      self._fill_crate_task:destroy()
      self._fill_crate_task = nil
   end
end

return FillCrateObserver
