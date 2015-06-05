local log = radiant.log.create_logger('fill_crate_observer')

local FillCrateObserver = class()

function FillCrateObserver:initialize(entity)
   self._sv._entity = entity

   self:_create_worker_tasks()
end

function FillCrateObserver:restore()
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
      end)
end

function FillCrateObserver:activate()
end

function FillCrateObserver:_create_worker_tasks()
   log:error('%s', self._sv._entity:get_component('unit_info'):get_player_id())
   local town = stonehearth.town:get_town('player_1')
   if town then 
      log:error('creating fill_crate task')
      self._fill_crate_task = town:create_task_for_group('stonehearth:task_group:restock', 'stonehearth:fill_crate', { crate = self._sv._entity })
         :set_source(self._sv._entity)
         :set_name('fill crate task')
         :set_priority(stonehearth.constants.priorities.simple_labor.RESTOCK_STOCKPILE)
      self._fill_crate_task:start()
   end
end


function FillCrateObserver:destroy()
end

return FillCrateObserver
