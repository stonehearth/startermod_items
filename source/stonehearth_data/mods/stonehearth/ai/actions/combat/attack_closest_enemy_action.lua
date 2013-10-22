local AttackClosestEnemy = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

AttackClosestEnemy.name = 'attack closest enemy'
AttackClosestEnemy.does = 'stonehearth:top'
AttackClosestEnemy.priority = 0

function AttackClosestEnemy:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._enemies = {}
   radiant.events.listen(radiant.events, 'gameloop', self, self.on_gameloop)
end

function AttackClosestEnemy:on_gameloop(e)
   radiant.events.unlisten(radiant.events, 'gameloop', self, self.on_gameloop)
   self:init_sight_sensor()
end

function AttackClosestEnemy:init_sight_sensor()
   assert(not self._sensor)

   local list = self._entity:get_component('sensor_list')
   if not list then
      return
   end

   self._sensor = list:get_sensor('sight')
   self.promise = self._sensor:get_contents():trace()

   self.promise:on_added(
      function (id)
         self:on_added_to_sensor(id)
      end
   ):on_removed(
      function (id)
         self:on_removed_from_sensor(id)
      end
   ) 
end

function AttackClosestEnemy:on_added_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)

   if radiant.entities.is_hostile(self._entity, entity) then         
      table.insert(self._enemies, entity_id)
      self._ai:set_action_priority(self, 100)
   end
end

function AttackClosestEnemy:on_removed_from_sensor(entity_id)
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

function AttackClosestEnemy:get_closest_enemy()
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

function AttackClosestEnemy:run(ai, entity)
   assert(#self._enemies > 0)
   ai:execute('stonehearth:attack', self:get_closest_enemy())
end

return AttackClosestEnemy