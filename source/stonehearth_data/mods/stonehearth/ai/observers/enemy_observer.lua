local log = radiant.log.create_logger('combat')

local EnemyObserver = class()

function EnemyObserver:initialize(entity, json)
   self._entity = entity

   local enable_combat = radiant.util.get_config('enable_combat', false)
   if not enable_combat then
      return
   end

   radiant.events.listen(self._entity, 'stonehearth:combat:battery', self, self._on_battery)
   self:_add_sensor_trace()
end

function EnemyObserver:_on_battery(context)
   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
   target_table:modify_score(context.attacker, context.damage)
end

function EnemyObserver:_add_sensor_trace()
   -- could configure sensor in json, but we want the radius to be the same as the sight radius
   local sight_radius = radiant.util.get_config('sight_radius', 64)
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('enemy_observer')

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

   if not target or not target:is_valid() then
      return
   end

   if self:_is_hostile(target) and self:_is_killable(target) then
      target_table = radiant.entities.get_target_table(self._entity, 'aggro')
      target_table:add(target)
   end
end

function EnemyObserver:_on_removed_from_sensor(target_id)
   local target = radiant.entities.get_entity(target_id)

   if not target or not target:is_valid() then
      return
   end

   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')

   target_table:remove(target)
end

function EnemyObserver:_is_hostile(target)
   -- fix the critter hack when we have a mapping table for hostilities
   local is_hostile = radiant.entities.is_hostile(self._entity, target) and
                      radiant.entities.get_faction(target) ~= 'critter'
   return is_hostile
end

function EnemyObserver:_is_killable(target)
   local attributes_component = target:get_component('stonehearth:attributes')
   if not attributes_component then 
      return false
   end

   local health = attributes_component:get_attribute('health')
   local is_killable = health and health > 0
   return is_killable
end

function EnemyObserver:destroy()
   self:_destroy_trace()
   radiant.events.unlisten(self._entity, 'stonehearth:combat:battery', self, self._on_battery)
end

function EnemyObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return EnemyObserver
