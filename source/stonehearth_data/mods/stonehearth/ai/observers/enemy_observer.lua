local log = radiant.log.create_logger('combat')

local EnemyObserver = class()

function EnemyObserver:initialize(entity, json)
   self._entity = entity

   local enable_combat = radiant.util.get_config('enable_combat', false)
   if not enable_combat then
      return
   end

   self:_add_sensor_trace()
   self:_listen_for_battery()
end

function EnemyObserver:_listen_for_battery()
   radiant.events.listen(self._entity, 'stonehearth:combat:battery', self, self._on_battery)
end

function EnemyObserver:_on_battery(context)
   local target_table = stonehearth.combat:get_target_table(self._entity, 'aggro')
   target_table:modify_score(context.attacker, context.damage)
end

function EnemyObserver:_add_sensor_trace()
   -- could configure sensor in json, but we want the radius to be the same as the sight radius
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

function EnemyObserver:destroy()
   self:_destroy_trace()
   self:_unregister_events()
end

function EnemyObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function EnemyObserver:_unregister_events()
   radiant.events.unlisten(self._entity, 'stonehearth:combat:battery', self, self._on_battery)
end

return EnemyObserver
