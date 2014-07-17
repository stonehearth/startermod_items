local log = radiant.log.create_logger('combat')

local AggroObserver = class()

function AggroObserver:initialize(entity, json)
   self._entity = entity
   self._observed_allies = {}
   self._ally_aggro_ratio = radiant.util.get_config('ally_aggro_ratio', 0.50)

   self:_add_sensor_trace()
end

function AggroObserver:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._trace = self._sensor:trace_contents('trace_allies')
      :on_added(function (target_id)
            self:_on_added_to_sensor(target_id)
         end)
      :on_removed(function (target_id)
            self:_on_removed_from_sensor(target_id)
         end)
      :push_object_state()
end

-- this may be called more than once for an entity, so make sure we can handle duplicates
function AggroObserver:_on_added_to_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity or not other_entity:is_valid() then
      return
   end

   -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
   if radiant.entities.is_friendly(other_entity, self._entity) then
      self:_observe_ally(other_entity)
   else
      if radiant.entities.is_hostile(other_entity, self._entity) then
         self:_add_hostile_to_aggro_table(other_entity)
      end
   end
end

function AggroObserver:_on_removed_from_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity or not other_entity:is_valid() then
      return
   end

   self:_unobserve_ally(other_entity)
end

function AggroObserver:_add_hostile_to_aggro_table(enemy)
   if self:_is_killable(enemy) then
      local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
      target_table:add(enemy)
   end
end

function AggroObserver:_observe_ally(ally)
   local ally_id = ally:get_id()

   if not self._observed_allies[ally_id] then
      -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
      radiant.events.listen(ally, 'stonehearth:combat:battery', self, self._on_ally_battery)
      self._observed_allies[ally_id] = ally
   end
end

function AggroObserver:_unobserve_ally(ally)
   local ally_id = ally:get_id()

   if self._observed_allies[ally_id] then
      -- TODO: make sure we unlisten when entities are about to be destroyed
      -- or have radiant.events periodically clean up listeners on destroyed entities
      radiant.events.unlisten(ally, 'stonehearth:combat:battery', self, self._on_ally_battery)
      self._observed_allies[ally_id] = nil
   end
end

-- note that an entity is considered an ally of itself
function AggroObserver:_on_ally_battery(context)
   if self:_is_killable(context.attacker) then
      local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
      if target_table then
         local aggro = self:_calculate_aggro(context)
         target_table:modify_score(context.attacker, aggro)
      end
   end
end

function AggroObserver:_calculate_aggro(context)
   local aggro = context.damage

   if context.target ~= self._entity then
      -- aggro from allies getting hit is less than self getting hit
      aggro = aggro * self._ally_aggro_ratio
   end

   return aggro
end

function AggroObserver:_is_killable(target)
   if not target or not target:is_valid() then
      return false
   end
   local attributes_component = target:get_component('stonehearth:attributes')
   if not attributes_component then 
      return false
   end

   local health = attributes_component:get_attribute('health')
   local is_killable = health and health > 0
   return is_killable
end

function AggroObserver:destroy()
   self:_destroy_trace()
end

function AggroObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return AggroObserver
