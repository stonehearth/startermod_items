local log = radiant.log.create_logger('combat')

local EnemyObserver = class()

function EnemyObserver:__init(entity)
end

function EnemyObserver:initialize(entity, json)
   self._entity = entity

   -- TOOD: support save
   -- wait one gameloop before looking for targets
   radiant.events.listen(radiant, 'stonehearth:gameloop', self, self._on_start)
end

function EnemyObserver:destroy()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function EnemyObserver:_on_start()
   local enable_combat = radiant.util.get_config('enable_combat', false)

   if enable_combat then
      self:_init_sight_sensor()
   end

   return radiant.events.UNLISTEN
end

function EnemyObserver:_init_sight_sensor()
   local sight_radius = radiant.util.get_config('sight_radius', 64)
   local sensor_list = self._entity:add_component('sensor_list')
   local sensor = sensor_list:get_sensor('enemy_observer')

   if self._sensor == nil then
      self._sensor = sensor_list:add_sensor('enemy_observer', sight_radius)
   end

   self._trace = self._sensor:trace_contents('trace_enemies')
      :on_added(function (target_id)
            self:_on_added_to_sensor(target_id)
         end)
      :on_removed(function (target_id)
            self:_on_removed_from_sensor(target_id)
         end)
      :push_object_state()
end

function EnemyObserver:_on_added_to_sensor(target_id)
   local target = radiant.entities.get_entity(target_id)
   local target_table

   if radiant.entities.is_hostile(self._entity, target) then
      target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')
      target_table:add(target)
   end
end

function EnemyObserver:_on_removed_from_sensor(target_id)
   local target = radiant.entities.get_entity(target_id)
   local target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')

   target_table:remove(target)
end

return EnemyObserver
