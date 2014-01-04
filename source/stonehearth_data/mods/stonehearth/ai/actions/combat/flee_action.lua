local event_service = require 'services.event.event_service'

local Flee = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

Flee.name = 'stonehearth:actions:flee'
Flee.does = 'stonehearth:top'
Flee.version = 1
Flee.priority = 0

function Flee:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._enemies = {}
   radiant.events.listen(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)
end

function Flee:on_gameloop()
   radiant.events.unlisten(radiant.events, 'stonehearth:gameloop', self, self.on_gameloop)
   self:init_sight_sensor()
end

function Flee:init_sight_sensor()
   assert(not self._sensor)

   local list = self._entity:get_component('sensor_list')
   if not list then
      return
   end

   self._sensor = list:get_sensor('sight')
   self.promise = self._sensor:trace_contents('flee')
                                 :on_added(
                                    function (id)
                                       self:on_added_to_sensor(id)
                                    end
                                 )
                                 :on_removed(
                                    function (id)
                                       self:on_removed_from_sensor(id)
                                    end
                                 ) 
end

function Flee:on_added_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)

   if radiant.entities.is_hostile(self._entity, entity) then         
      table.insert(self._enemies, entity_id)
      self._ai:set_action_priority(self, 90)
   end
end

function Flee:on_removed_from_sensor(entity_id)
   for i, enemy_id in ipairs(self._enemies) do
      if enemy_id == entity_id then
         table.remove(self._enemies, i)
         break
      end
   end
   
   if #self._enemies == 0 then
      self._ai:set_action_priority(self, 0)
   end
end

function Flee:get_closest_enemy()
   assert(#self._enemies > 0)

   local closest_distance = 99999
   local closest_enemy = nil

   for _, enemy_id in ipairs(self._enemies) do
      local enemy = radiant.entities.get_entity(enemy_id)
      local distance = radiant.entities.distance_between(self._entity, enemy)
      if distance < closest_distance then
         closest_enemy = enemy
      end
   end

   return closest_enemy
end

function Flee:run(ai, entity)
   assert(#self._enemies > 0)
   radiant.entities.drop_carrying_on_ground(entity)
   radiant.entities.set_posture(entity, 'panic')

   local entity_name = radiant.entities.get_display_name(entity)
   event_service:add_entry(entity_name .. ' is in trouble!')

   ai:execute('stonehearth:run_away_from_entity', self:get_closest_enemy())
end

function Flee:stop(ai, entity)
   radiant.entities.unset_posture(self._entity, 'panic')
end
return Flee