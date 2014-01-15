local event_service = stonehearth.events

local Flee = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

Flee.name = 'flee danger'
Flee.does = 'stonehearth:top'
Flee.args = {}
Flee.version = 2
Flee.priority = 8

function Flee:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._enemies = {}
end

function Flee:start_thinking(ai, entity)
   assert(not self._sensor)

   local list = entity:get_component('sensor_list')
   if not list then
      return
   end
   self._sensor = list:get_sensor('sight')
   self.promise = self._sensor:trace_contents('flee')
                                 :on_added(
                                    function (id, foe)
                                       self:on_added_to_sensor(ai, entity, id, foe)
                                    end
                                 )
                                 :on_removed(
                                    function (id)
                                       self:on_removed_from_sensor(ai, entity, id)
                                    end
                                 ) 
end

function Flee:on_added_to_sensor(ai, entity, foe_id, foe)
   if radiant.entities.is_hostile(entity, foe) then
      table.insert(self._enemies, foe_id)
      ai:set_think_output({})
   end
end

function Flee:on_removed_from_sensor(ai, entity, foe_id)
   if #self._enemies > 0 then
      for i, enemy_id in ipairs(self._enemies) do
         if enemy_id == foe_id then
            table.remove(self._enemies, i)
            break
         end
      end
      
      if #self._enemies == 0 then
         ai:revoke_think_output()
      end
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