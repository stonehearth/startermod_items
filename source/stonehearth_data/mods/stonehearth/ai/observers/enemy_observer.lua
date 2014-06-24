local log = radiant.log.create_logger('combat')

local EnemyObserver = class()

function EnemyObserver:initialize(entity, json)
   self._entity = entity

   self:_add_sensor_trace()
end

function EnemyObserver:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._trace = self._sensor:trace_contents('trace_enemies')
      :on_added(function (target_id)
            self:_on_added_to_sensor(target_id)
         end)
      :on_removed(function (target_id)
            self:_on_removed_from_sensor(target_id)
         end)
      :push_object_state()
end

-- this may be called more than once for an entity, so make sure we can handle duplicates
function EnemyObserver:_on_added_to_sensor(target_id)
   local target = radiant.entities.get_entity(target_id)
   local target_table

   if not target or not target:is_valid() then
      return
   end

   if radiant.entities.is_hostile(self._entity, target) and self:_is_killable(target) then
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
end

function EnemyObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return EnemyObserver
