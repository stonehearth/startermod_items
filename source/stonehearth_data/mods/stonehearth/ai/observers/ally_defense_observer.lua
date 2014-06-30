local log = radiant.log.create_logger('combat')

local AllyDefenseObserver = class()

function AllyDefenseObserver:initialize(entity, json)
   self._entity = entity
   self._allies_in_range = {}

   self:_add_sensor_trace()
end

function AllyDefenseObserver:_add_sensor_trace()
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
function AllyDefenseObserver:_on_added_to_sensor(id)
   if self._allies_in_range[id] then
      -- entity already tracked
      return
   end

   local other_entity = radiant.entities.get_entity(id)

   if not other_entity or not other_entity:is_valid() then
      return
   end

   -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
   if radiant.entities.is_friendly(self._entity, other_entity) then
      -- TODO: verify that self is added to sensor
      radiant.events.listen(other_entity, 'stonehearth:combat:battery', self, self._on_ally_battery)
   end

   self._allies_in_range[id] = other_entity
end

function AllyDefenseObserver:_on_removed_from_sensor(id)
   self._allies_in_range[id] = nil

   local other_entity = radiant.entities.get_entity(id)

   if not other_entity or not other_entity:is_valid() then
      return
   end

   -- TODO: make sure we unlisten when entities are about to be destroyed
   radiant.events.unlisten(other_entity, 'stonehearth:combat:battery', self, self._on_ally_battery)
end

-- note that an entity is considered an ally of itself
function AllyDefenseObserver:_on_ally_battery(context)
   if self:_is_killable(context.attacker) then
      local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
      if target_table then
         target_table:modify_score(context.attacker, context.damage)
      end
   end
end

function AllyDefenseObserver:_is_killable(target)
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

function AllyDefenseObserver:destroy()
   self:_destroy_trace()
end

function AllyDefenseObserver:_destroy_trace()
   -- TODO: confirm trace and sensor are destroyed when entity is destroyed
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return AllyDefenseObserver
