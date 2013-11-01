local FollowDestination = class()

FollowDestination.name = 'follow a destination'
FollowDestination.does = 'stonehearth:follow_destination'
FollowDestination.priority = 1

function FollowDestination:run(ai, entity, path, effect_name)
   local speed = entity:add_component('stonehearth:attributes'):get_attribute('speed')
   
   if speed == nil then
      speed = 1.0
   else 
      speed = speed / 100
   end

   if not effect_name then
      effect_name = 'run'
   end

   if not self._effect then
      self._effect = radiant.effects.run_effect(entity, effect_name)
   end

   local arrived_fn = function()
      ai:resume()
   end  
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, 0, arrived_fn)
   ai:suspend()
   
   --self._effect:stop()
   --self._effect = nil
end

function FollowDestination:stop(ai, entity)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end

return FollowDestination