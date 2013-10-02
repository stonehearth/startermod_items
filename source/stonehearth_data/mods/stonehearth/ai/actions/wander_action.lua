local WanderAction = class()

WanderAction.name = 'stonehearth.actions.wander'
WanderAction.does = 'stonehearth.top'
WanderAction.priority = 2

function WanderAction:__init(ai, entity)
   self._ai = ai
   self:set_next_run()
   local leash = radiant.entities.get_entity_data(entity, 'stonehearth:wander_leash')

   if leash and leash.radius then
      self._wander_radius = leash.radius
   else 
      self._wander_radius = 0
   end

   radiant.events.listen('radiant.events.very_slow_poll', self)
end

WanderAction['radiant.events.very_slow_poll'] = function(self)
   if self._runInterval == 0 then
      self._ai:set_action_priority(self, 2)
      self:set_next_run();
   end

   self._runInterval = self._runInterval - 1
end

function WanderAction:run(ai, entity)

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

   ai:execute('stonehearth.goto_location', location)
   ai:set_action_priority(self, 0)
end

function WanderAction:set_next_run(ai, entity)
   self._runInterval = math.random(4, 8)
end

return WanderAction