local FollowPathAction = class()

FollowPathAction.name = 'stonehearth.actions.follow_path'
FollowPathAction.does = 'stonehearth.activities.follow_path'
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path)
   -- todo: get speed based on entity and posture
   local speed = 1

   -- todo: get effect name based on posture
   local effect_name = 'run'
   if radiant.entities.is_carrying(entity) then
      effect_name = 'carry_walk'
   end
   self._effect = radiant.effects.run_effect(entity, effect_name)

   local arrived_fn = function()
      ai:resume()
   end  
   self._mover = native:create_follow_path(entity, speed, path, 0, arrived_fn)
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