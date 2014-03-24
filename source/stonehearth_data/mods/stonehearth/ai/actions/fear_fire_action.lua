local FearFire = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

FearFire.name = 'fear fire'
FearFire.does = 'stonehearth:top'
FearFire.version = 1
FearFire.priority = 0

function FearFire:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   self._fires = {}
   radiant.events.listen(radiant, 'stonehearth:gameloop', self, self.on_gameloop)
end

function FearFire:on_gameloop(_)
   radiant.events.unlisten(radiant, 'stonehearth:gameloop', self, self.on_gameloop)
   self:init_sight_sensor()
end

function FearFire:init_sight_sensor()
   assert(not self._sensor)

   local list = self._entity:get_component('sensor_list')
   if not list then
      return
   end

   self._sensor = list:get_sensor('sight')
   self.promise = self._sensor:trace_contents('fear fire action')
                                 :on_added(function (id)
                                    self:on_added_to_sensor(id)
                                 end)
                                 :on_removed(function (id)
                                    self:on_removed_from_sensor(id)
                                 end) 
end

function FearFire:on_added_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)
   local firepit_component = entity:get_component('stonehearth:firepit')

   if firepit_component and firepit_component:is_lit() then
      table.insert(self._fires, entity_id)
      self._ai:set_action_priority(self, 200)
   end
end

function FearFire:on_removed_from_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)

   for i, enemy_id in ipairs(self._fires) do
      if enemy_id == entity_id then
         table.remove(self._fires, i)
         break
      end
   end
   
   if #self._fires == 0 then
      self._ai:set_action_priority(self, 0)
   end

end

function FearFire:run(ai, entity)
   assert(#self._fires > 0)

   local fire_entity = radiant.entities.get_entity(self._fires[1])
   --ai:execute('stonehearth:run_away_from_entity', fire_entity)
   --do nothing and stalk

   ai:execute('stonehearth:idle:breathe')
end

return FearFire