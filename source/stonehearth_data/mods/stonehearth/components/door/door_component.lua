local DoorComponent = class()

function DoorComponent:__init(entity)
   self._entity = entity
   self._info = {}
   self._in_sensor = {}
   self._tracked_entities = {}
end

function DoorComponent:extend(json)
   assert(json.sensor)

   local sensor_list = self._entity:get_component('sensor_list')
   if sensor_list then
      self._sensor = sensor_list:get_sensor(json.sensor)

      self.promise = self._sensor:trace_contents('open door')
                                    :on_added(
                                       function (id, entity)
                                          self:on_added_to_sensor(id, entity)
                                       end
                                    )
                                    :on_removed(
                                       function (id)
                                          self:on_removed_to_sensor(id)
                                       end
                                    ) 

   end

   self._effect = radiant.effects.run_effect(self._entity, 'closed')
end

function DoorComponent:on_added_to_sensor(entity_id, entity)
   if self:_valid_entity(entity) then
      if #self._tracked_entities == 0 then
         -- if this is in our faction, open the door
         self:open_door();
      end

      self:_track_entity(entity_id)
   end
end

function DoorComponent:on_removed_to_sensor(entity_id)
    local entity = radiant.entities.get_entity(entity_id)
    if self:_valid_entity(entity) then
         self:_untrack_entity(entity_id)
        if #self._tracked_entities == 0 then
            self:close_door()
        end
    end
end

function DoorComponent:_track_entity(id)
   for i, tracked_id in ipairs(self._tracked_entities) do
      if id == tracked_id then
         return
      end
   end

   table.insert(self._tracked_entities, id)
end

function DoorComponent:_untrack_entity(id)
   for i, tracked_id in ipairs(self._tracked_entities) do
      if id == tracked_id then
         table.remove(self._tracked_entities, i)
         return
      end
   end
end

function DoorComponent:open_door()
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end

   self._effect = radiant.effects.run_effect(self._entity, 'open')
end

function DoorComponent:close_door()
    if self._effect then
        self._effect:stop()
        self._effect = nil
    end

    self._effect = radiant.effects.run_effect(self._entity, 'close')
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

   --if radiant.entities.get_faction(entity) ~= radiant.entities.get_faction(self._entity) then
   if radiant.entities.get_faction(entity) ~= 'civ' then
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