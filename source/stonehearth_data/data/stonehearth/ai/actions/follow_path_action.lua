local FollowPathAction = class()

FollowPathAction.name = 'stonehearth.actions.follow_path'
FollowPathAction.does = 'stonehearth.activities.follow_path'
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path)
   -- todo: get speed based on entity and posture
   local speed = 1

   -- todo: get effect name based on posture
   local effect_name = 'run'

   self._effect = radiant.effects.run_effect(entity, effect_name)
   self._mover = native:create_follow_path(entity, speed, path, 0)
   ai:wait_until(function()
         return self._mover:finished()
      end)      
   
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