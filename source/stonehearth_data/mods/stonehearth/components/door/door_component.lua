local DoorComponent = class()

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

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
            self:_add_collision_shape()
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

function DoorComponent:_add_collision_shape()
   local portal = self._entity:get_component('stonehearth:portal')
   if portal then
      local mob = self._entity:add_component('mob')
      local mgs = self._entity:add_component('movement_guard_shape')

      local region2 = portal:get_portal_region()
      local region3 = mgs:get_region()
      if not region3 then
         region3 = radiant.alloc_region3()
         mgs:set_region(region3)
      end
      region3:modify(function(cursor)
            cursor:clear()
            for rect in region2:each_cube() do
               cursor:add_unique_cube(Cube3(Point3(rect.min.x, rect.min.y,  0),
                                            Point3(rect.max.x, rect.max.y,  1)))
            end
         end)

      mgs:set_guard_cb(function(entity, location)
            return stonehearth.player:are_players_friendly(self._entity, entity)
         end)
   end
end

function DoorComponent:_add_movement_guard(region)
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
         :set_cleanup_on_finish(false)
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

   if radiant.entities.get_player_id(entity) ~= radiant.entities.get_player_id(self._entity) then
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