local DoorComponent = class()

function DoorComponent:initialize(entity, json)
   self._entity = entity
   self._tracked_entities = {}

   self._sv = self.__saved_variables:get_data()
   if not self._sv.sensor_name then
      self._sv.sensor_name = json.sensor  
      self.__saved_variables:mark_changed()
   end
   if self._sv.sensor_name then
      radiant.events.listen_once(self._entity, 'radiant:entity:post_create', function()
            self:_trace_sensor()
         end)
   end
end

function DoorComponent:destroy()
   if self._open_effect then
      self._open_effect:stop()
      self._open_effect = nil
   end
   if self._close_effect then
      self._close_effect:stop()
      self._close_effect = nil
   end
end

function DoorComponent:_trace_sensor()
   local sensor_list = self._entity:get_component('sensor_list')
   local sensor = sensor_list:get_sensor(self._sv.sensor_name)
   if sensor then
      self._sensor_trace = sensor:trace_contents('door')
                                       :on_added(function (id, entity)
                                             self:_on_added_to_sensor(id, entity)
                                          end)
                                       :on_removed(function (id)
                                             self:_on_removed_to_sensor(id)
                                          end)
                                       :push_object_state()
   end
end

function DoorComponent:_on_added_to_sensor(id, entity)
   if self:_valid_entity(entity) then
      if not next(self._tracked_entities) then
         -- if this is in our faction, open the door
         self:_open_door();
      end
      self._tracked_entities[id] = entity
   end
end

function DoorComponent:_on_removed_to_sensor(id)
   self._tracked_entities[id] = nil
   if not next(self._tracked_entities) then
      self:_close_door()
   end
end

function DoorComponent:_open_door()
   if self._close_effect then
      self._close_effect:stop()
      self._close_effect = nil
   end
   if not self._open_effect then
      self._open_effect = radiant.effects.run_effect(self._entity, 'open')
   end
end

function DoorComponent:_close_door()
   if self._open_effect then
      self._open_effect:stop()
      self._open_effect = nil
   end
   if not self._close_effect then
      self._close_effect = radiant.effects.run_effect(self._entity, 'close')
   end
end

function DoorComponent:_valid_entity(entity)
   if not entity then
      return false
   end

   -- xxx, bit of a hack? open the door only for things with an ai   
   if not entity:get_component('stonehearth:ai') then
      return false
   end

   if entity:get_id() == self._entity:get_id() then
      return false
   end

   if radiant.entities.get_faction(entity) ~= radiant.entities.get_faction(self._entity) then
      return false
   end
   
   --[[
   if not mob_component or not mob_component:get_moving() then
      return false
   end
   ]]

   return true
end

return DoorComponent