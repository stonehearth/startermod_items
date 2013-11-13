local Point3 = _radiant.csg.Point3

local Wander = class()

Wander.name = 'wander'
Wander.does = 'stonehearth:top'
Wander.priority = 0

function Wander:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   self._polls_before_move = math.random(5, 8)
   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self.on_poll)
end

function Wander:on_poll()
   if self._polls_before_move == 0 then
      if (self._entity:get_component('stonehearth:leash')) then
         self._ai:set_action_priority(self, 2)
      end
      self._polls_before_move = math.random(5, 8)
   else
      self._polls_before_move = self._polls_before_move - 1
   end
end

function Wander:run(ai, entity)
   local leash_component = entity:get_component('stonehearth:leash')
   local leash_location = leash_component:get_location()
   local entity_location = radiant.entities.get_location_aligned(entity)

   local radius = 3
   local dx = math.random(-3, 3)
   local dz = math.random(-3, 3)

   if ((entity_location.x + dx > leash_location.x + radius) or 
       (entity_location.x + dx < leash_location.x - radius)) then

       dx = dx * -1;
   end

   if ((entity_location.z + dz > leash_location.z + radius) or 
       (entity_location.x + dz < leash_location.z - radius)) then

       dz = dz * -1;
   end   

   local destination = Point3(leash_location)
   destination.x = destination.x + dx
   destination.z = destination.z + dz
   
   ai:execute('stonehearth:goto_location', destination)
   self._ai:set_action_priority(self, 0)
end

function Wander:run_old_wander_behavior(ai, entity)

   -- set the initial location, so we know how far to wander at max
   if not self._initial_location then
      self._initial_location = radiant.entities.get_location_aligned(entity)
   end

   local location = radiant.entities.get_location_aligned(entity)
   local radius = self._wander_radius

   local leash_location = self._initial_location
   local xMax = leash_location.x + radius - location.x
   local xMin = leash_location.x - radius + location.x

   location.x = location.x + math.random(-3, 3)
   location.z = location.z + math.random(-3, 3)

   -- if we bounce outside of the leash box, bounce back to the middle
   if (location.x > leash_location.x + radius) or (location.x < leash_location.x - radius) then
      location.x = leash_location.x + math.random(-1, 1)
   end

   if (location.z > leash_location.z + radius) or (location.z < leash_location.z - radius) then
      location.z = leash_location.z + math.random(-1, 1)
   end

   ai:execute('stonehearth:goto_location', location)
   ai:set_action_priority(self, 0)
end

return Wander