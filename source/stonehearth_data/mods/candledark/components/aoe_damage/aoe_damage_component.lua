local AoeDamageComponent = class()

function AoeDamageComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   self._sensor_name = json.sensor_name
   self._damage_amount = json.damage_amount or 0
   self._damage_effect = json.damage_effect 
   self._aoe_effect = json.aoe_effect

   self._tracked_entities = {}
   
   if self._sensor_name then
      self._poll_listener = radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._on_poll)
      radiant.events.listen_once(self._entity, 'radiant:entity:post_create', function()
            self:_trace_sensor()
         end)
   end   
end

-- Deal damage to everyone within my sensor
function AoeDamageComponent:_on_poll()
   
   -- If an aoe effect has been specified and there's at least one entry in the
   -- tracked entities table, run the aoe effect on my entity
   if self._aoe_effect and next(self._tracked_entities) then
      radiant.effects.run_effect(self._entity, self._aoe_effect)
   end

   -- For all the hostile entities being tracked, play the damage effect and deal
   -- damage
   for id, entity in pairs(self._tracked_entities) do
      -- play the damage effect
      if self._damage_effect then
         radiant.effects.run_effect(entity, self._damage_effect)
      end

      -- deal damage to the entity by settings hits health attribute
      local attributes = entity:get_component('stonehearth:attributes')
      if attributes ~= nil then
         local health = attributes:get_attribute('health')
         health = health - self._damage_amount
         attributes:set_attribute('health', health)
      end
   end
end

-- Track entities as they enter and leave the sensor
function AoeDamageComponent:_trace_sensor()
   local sensor_list = self._entity:get_component('sensor_list')
   local sensor = sensor_list:get_sensor(self._sensor_name)
   if sensor then
      self._sensor_trace = sensor:trace_contents('aoe damage component')
                                    :on_added(function (id, entity)
                                          self:_on_added_to_sensor(id, entity)
                                       end)
                                    :on_removed(function (id)
                                          self:_on_removed_to_sensor(id)
                                       end)
                                    :push_object_state()
   end
end

-- When an entity enters the sensor, keep track of it if it's hostile. Every
-- so often we will deal damage to all tracked entities
function AoeDamageComponent:_on_added_to_sensor(id, entity)
   if radiant.entities.is_hostile(entity, self._entity) then
      self._tracked_entities[id] = entity
   end
end

-- When an entity leaves the sensor, stop tracking it
function AoeDamageComponent:_on_removed_to_sensor(id)
   self._tracked_entities[id] = nil
end


-- Cleanup all listeners when the component is destroyed
function AoeDamageComponent:destroy()
   self._poll_listener:destroy()
   self._poll_listener = nil
end

return AoeDamageComponent
