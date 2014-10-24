local AoeDamageComponent = class()

-- initializes the component.
--
--    @param entity - the entity this component is for
--    @param json - the blob of json for this component in the entity definition
--                  file
--
function AoeDamageComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   self._sensor_name = json.sensor_name
   self._damage_amount = json.damage_amount or 0
   self._damage_effect = json.damage_effect
   self._aoe_effect = json.aoe_effect

   self._tracked_entities = {}
   
   if self._sensor_name then
      -- the order components are initialized is undefined.  wait until the entity
      -- is fully created to try to access the sensor component.
      radiant.events.listen_once(self._entity, 'radiant:entity:post_create', function()
            self:_trace_sensor()
         end)
   end   
end

-- called when the component is removed or the entity owning the component is destroyed
--
function AoeDamageComponent:destroy()
   if self._sensor_trace then
      self._sensor_trace:destroy()
      self._sensor_trace = nil
   end
end

-- Deal damage to everyone within my sensor
--
function AoeDamageComponent:_do_aoe_damage()
   
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
--
function AoeDamageComponent:_trace_sensor()
   local sensor_list = self._entity:get_component('sensor_list')
   if sensor_list then
      local sensor = sensor_list:get_sensor(self._sensor_name)
      if sensor then
         self._sensor_trace = sensor:trace_contents('aoe damage component')
                                       :on_added(function (id, entity)
                                             -- called whenever and entity enter sensor range
                                             self:_on_added_to_sensor(id, entity)
                                          end)
                                       :on_removed(function (id)
                                             -- called whenever and entity exits sensor range
                                             self:_on_removed_to_sensor(id)
                                          end)
                                       :push_object_state()
      end
   end
end

-- When an entity enters the sensor, keep track of it if it's hostile. Every
-- so often we will deal damage to all tracked entities
--
function AoeDamageComponent:_on_added_to_sensor(id, entity)

   -- track the entity if it's hostile
   if radiant.entities.is_hostile(entity, self._entity) then
      self._tracked_entities[id] = entity
   end

   -- if there are any entities being tracked and we haven't set the aoe tick timer, then do so.
   if next(self._tracked_entities) and self._aoe_timer == nil then
      self._aoe_timer = stonehearth.calendar:set_interval('10m',  function()
                              self:_do_aoe_damage()
                           end)
   end
end

-- When an entity leaves the sensor, stop tracking it
function AoeDamageComponent:_on_removed_to_sensor(id)
   -- stop tracking the entity
   self._tracked_entities[id] = nil

   -- if there are aren't any more entities being tracked, clean up the aoe timer
   if next(self._tracked_entities) == nil and self._aoe_timer ~= nil then
      self._aoe_timer:destroy()
      self._aoe_timer = nil
   end

end

return AoeDamageComponent
