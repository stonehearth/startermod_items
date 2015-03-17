local log = radiant.log.create_logger('combat')

local AggroObserver = class()

function AggroObserver:initialize(entity)
   self._log = radiant.log.create_logger('combat')
                           :set_prefix('aggro_observer')
                           :set_entity(entity)

   self._entity = entity
   self._observed_allies = {}
   self._observed_allies_listeners = {}
   self._ally_aggro_ratio = radiant.util.get_config('ally_aggro_ratio', 0.50)

   self:_add_sensor_trace()
end

function AggroObserver:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._trace = self._sensor:trace_contents('trace allies')
      :on_added(function (id, entity)
            self:_on_added_to_sensor(id, entity)
         end)
      :on_removed(function (id)
            self:_on_removed_from_sensor(id)
         end)
      :push_object_state()
end

-- this may be called more than once for an entity, so make sure we can handle duplicates
function AggroObserver:_on_added_to_sensor(id, other_entity)
   if not other_entity or not other_entity:is_valid() then
      return
   end
   self._log:spam('%s entered sight sensor', other_entity)

   -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
   -- TODO: only observe units, not structures that have factions
   if radiant.entities.is_friendly(other_entity, self._entity) then
      self:_observe_ally(other_entity)
      return
   end
   if radiant.entities.is_hostile(other_entity, self._entity) then
      self:_add_hostile_to_aggro_table(other_entity)
   end
end

function AggroObserver:_on_removed_from_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity or not other_entity:is_valid() then
      return
   end
   self._log:spam('%s left sight sensor', other_entity)

   self:_unobserve_ally(other_entity)
end

function AggroObserver:_add_hostile_to_aggro_table(enemy)
   if not self:_is_killable(enemy) then
      return
   end

   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
   local current = target_table:get_value(enemy)
   if current == nil then
      -- the first time we see the enemy, we bump their aggro up by their menace
      -- stat
      local menace = radiant.entities.get_attribute(enemy, 'menace', 1)
      target_table:modify_value(enemy, menace)
   end
end

function AggroObserver:_observe_ally(ally)
   local ally_id = ally:get_id()

   if not self._observed_allies[ally_id] then
      -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
      local listener = radiant.events.listen(ally, 'stonehearth:combat:battery', self, self._on_ally_battery)
      self._observed_allies_listeners[ally_id] = listener
      self._observed_allies[ally_id] = ally
   end
end

function AggroObserver:_unobserve_ally(ally)
   local ally_id = ally:get_id()

   if self._observed_allies[ally_id] then
      -- TODO: make sure we unlisten when entities are about to be destroyed
      -- or have radiant.events periodically clean up listeners on destroyed entities
      self._observed_allies_listeners[ally_id]:destroy()
      self._observed_allies_listeners[ally_id] = nil
      self._observed_allies[ally_id] = nil
   end
end

-- note that an entity is considered an ally of itself
function AggroObserver:_on_ally_battery(context)
   if not self:_is_killable(context.attacker) then
      return
   end

   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
   if target_table then
      local aggro = context.damage
      if context.target ~= self._entity then
         -- aggro from allies getting hit is less than self getting hit
         aggro = aggro * self._ally_aggro_ratio
      end
      target_table:modify_value(context.attacker, aggro)
   end
end

function AggroObserver:_is_killable(target)
   if not target or not target:is_valid() then
      self._log:spam('target is invalid in :_is_killable().  returning false')
      return false
   end
   local attributes_component = target:get_component('stonehearth:attributes')
   if not attributes_component then 
      self._log:spam('%s has no attributes component :_is_killable().  returning false', target)
      return false
   end

   local health = attributes_component:get_attribute('health')
   local is_killable = health and health > 0

   self._log:spam('%s health is %d in :_is_killable().  returning %s', target, tostring(health), tostring(is_killable))
   return is_killable
end

function AggroObserver:destroy()
   self:_destroy_trace()

   for _, listener in pairs(self._observed_allies_listeners) do
      listener:destroy()
   end
   self._observed_allies_listeners = nil
end

function AggroObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return AggroObserver
