local log = radiant.log.create_logger('fill_crate_observer')

local FillCrateObserver = class()

function FillCrateObserver:initialize(entity)
   self._sv.entity = entity
end

function FillCrateObserver:activate()
   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)

   self._parent_trace = self._sv.entity:add_component('mob'):trace_parent('fill crate observer parent')
      :on_changed(function()
            self:_on_parent_changed(radiant.entities.get_parent(self._sv.entity))
         end)

   if player_id ~= '' then
      self:_create_worker_task()
   else
      self._unit_info_trace = self._sv.entity:add_component('unit_info'):trace_player_id('fill crate observer unit id')
         :on_changed(function()
               self._unit_info_trace:destroy()
               self._unit_info_trace = nil
               self:_create_worker_task()
            end)

   end
end

function FillCrateObserver:_on_parent_changed(new_parent)
   if new_parent then
      if new_parent:get_component('terrain') then
         self:_create_worker_task()
      else
         self:_destroy_worker_task()
      end
   else
      self:_destroy_worker_task()
   end
end

function FillCrateObserver:_create_worker_task()
   if self._fill_crate_task then
      return
   end

   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)
   local town = stonehearth.town:get_town(player_id)
   if town then 
      self._fill_crate_task = town:create_task_for_group('stonehearth:task_group:restock', 'stonehearth:fill_crate', { crate = self._sv.entity })
         :set_source(self._sv.entity)
         :set_name('fill crate task')
         :set_priority(stonehearth.constants.priorities.simple_labor.RESTOCK_STOCKPILE)
      self._fill_crate_task:start()
   end
end

function FillCrateObserver:_destroy_worker_task()
   if self._fill_crate_task then
      self._fill_crate_task:destroy()
      self._fill_crate_task = nil
   end
end


function FillCrateObserver:destroy()
   if self._unit_info_trace then
      self._unit_info_trace:detroy()
      self._unit_info_trace = nil
   end

   if self._parent_trace then
      self._parent_trace:detroy()
      self._parent_trace = nil
   end

   self:_destroy_worker_task()
end

return FillCrateObserver
