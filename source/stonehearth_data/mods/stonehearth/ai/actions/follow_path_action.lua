local FollowPathAction = class()

FollowPathAction.name = 'stonehearth.actions.follow_path'
FollowPathAction.does = 'stonehearth.follow_path'
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path)
   local speed = 1.0  
   self._effect = radiant.effects.run_effect(entity, 'run')

   local arrived_fn = function()
      ai:resume()
   end  
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, 0, arrived_fn)
   ai:suspend()
   
   self._effect:stop()
   self._effect = nil
end

function FollowPathAction:stop(ai, entity)
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end

return FollowPathAction