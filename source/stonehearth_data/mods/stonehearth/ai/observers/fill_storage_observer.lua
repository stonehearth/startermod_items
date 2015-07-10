local log = radiant.log.create_logger('fill_storage_observer')

local FillStorageObserver = class()

function FillStorageObserver:initialize(entity)
   self._sv.entity = entity
end

function FillStorageObserver:activate()
   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)

   self._parent_trace = self._sv.entity:add_component('mob'):trace_parent('fill storage observer parent')
      :on_changed(function(e)
            self:_on_parent_changed(radiant.entities.get_parent(self._sv.entity))
         end)

   if player_id ~= '' then
      self:_update_worker_task()
   else
      self._unit_info_trace = self._sv.entity:add_component('unit_info'):trace_player_id('fill storage observer unit id')
         :on_changed(function()
               self._unit_info_trace:destroy()
               self._unit_info_trace = nil
               self:_update_worker_task()
            end)

   end

   self._filter_listener = radiant.events.listen(self._sv.entity, 'stonehearth:storage:filter_changed', self, self._on_filter_changed)
end

function FillStorageObserver:_on_filter_changed(filter_component, newly_filtered, newly_passed)
   self:_destroy_worker_task()
   self:_update_worker_task()
end

function FillStorageObserver:_on_parent_changed(new_parent)
   self:_update_worker_task()
end

function FillStorageObserver:_update_worker_task()
   local parent = radiant.entities.get_parent(self._sv.entity)
   if not parent or self:_is_in_container() then
      self:_destroy_worker_task()
   else
      self:_create_worker_task()
   end
end

function FillStorageObserver:_create_worker_task()
   if self._fill_storage_task then
      return
   end

   local player_id = radiant.entities.get_player_id_from_entity(self._sv.entity)
   local town = stonehearth.town:get_town(player_id)
   if town then 
      self._fill_storage_task = town:create_task_for_group('stonehearth:task_group:restock', 'stonehearth:fill_storage', { storage = self._sv.entity })
         :set_source(self._sv.entity)
         :set_name('fill storage task')
         :set_priority(stonehearth.constants.priorities.simple_labor.RESTOCK_STOCKPILE)
      self._fill_storage_task:start()
   end
end

function FillStorageObserver:_destroy_worker_task()
   if self._fill_storage_task then
      self._fill_storage_task:destroy()
      self._fill_storage_task = nil
   end
end

function FillStorageObserver:_is_in_container()
   local player_id = self._sv.entity:add_component('unit_info'):get_player_id()
   local inventory = stonehearth.inventory:get_inventory(player_id)
   local result = inventory:container_for(self._sv.entity) ~= nil
   return result
end

function FillStorageObserver:destroy()
   if self._unit_info_trace then
      self._unit_info_trace:destroy()
      self._unit_info_trace = nil
   end

   if self._parent_trace then
      self._parent_trace:destroy()
      self._parent_trace = nil
   end

   self:_destroy_worker_task()
end

return FillStorageObserver
