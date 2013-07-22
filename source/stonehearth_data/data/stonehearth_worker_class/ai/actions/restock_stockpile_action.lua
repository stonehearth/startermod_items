local RestockStockpileAction = class()

RestockStockpileAction.name = 'stonehearth.actions.restock_stockpile'
RestockStockpileAction.does = 'stonehearth.activities.top'
RestockStockpileAction.priority = 0


function RestockStockpileAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   
   local faction = self._entity:get_component('unit_info'):get_faction()
   self._scheduler = radiant.mods.require('/stonehearth_worker_class').get_worker_scheduler(faction)

   self:_wait_for_next_task()
end

function RestockStockpileAction:_wait_for_next_task()
   if self._task then
      self._scheduler:abort_worker_task(self._task)
      self._task = nil
   end

   local dispatch_fn = function(priority, action, ...)
      self._task = { action, ... }
      self._ai:set_action_priority(self, priority)
   end

   self._ai:set_action_priority(self, 0)
   self._scheduler:add_worker(self._entity, dispatch_fn)
end

function RestockStockpileAction:run(ai, entity, ...)
   ai:execute(unpack(self._task))
   --[[
   ai:execute('stonehearth.activities.follow_path', self._path_to_item)
   ai:execute('stonehearth.activities.pickup_item', self._item)
   ai:execute('stonehearth.activities.follow_path', self._path_to_stockpile)
   ai:execute('stonehearth.activities.drop_carrying', self._drop_location)
   ]]
   self._task = nil
end

function RestockStockpileAction:stop()
   self:_wait_for_next_task()
end

return RestockStockpileAction
