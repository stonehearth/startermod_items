local FollowLeash = class()

FollowLeash.name = 'wander'
FollowLeash.does = 'stonehearth:top'
FollowLeash.priority = 0

function FollowLeash:__init(ai, entity)
   self._ai = ai
   
   --self:set_next_run()
   local leash = entity:add_component('stonehearth:leash')
   local promise = leash:get_data_store():trace('follow leash ai')
   promise:on_changed(function() 
      ai:set_action_priority(self, 2)
   end)
end

function FollowLeash:run(ai, entity)
   local leash_component = entity:add_component('stonehearth:leash')
   local location = leash_component:get_location()

   if location then
      ai:execute('stonehearth:goto_location', location)
   end
end

function FollowLeash:run_old_wander_behavior(ai, entity)

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

function FollowLeash:set_next_run(ai, entity)
   self._runInterval = math.random(4, 8)
end

return FollowLeash