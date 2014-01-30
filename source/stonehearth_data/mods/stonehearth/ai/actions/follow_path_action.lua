local FollowPathAction = class()

FollowPathAction.name = 'follow path'
FollowPathAction.does = 'stonehearth:follow_path'
FollowPathAction.version = 1
FollowPathAction.priority = 1

function FollowPathAction:run(ai, entity, path, effect_name)
   if path:is_empty() then
      return
   end
   
   self._path_trace = radiant.entities.trace_location(path:get_destination(), 'watching path')
      :on_changed(function()
         ai:abort('path destination changed')
      end)
      :on_destroyed(function()
         ai:abort('path destination destroyed')
      end)

   local postures = entity:get_component('stonehearth:posture')
   if postures then
      self._postures_trace = postures:trace('follow path')
         :on_changed(function()
            ai:abort('posture changed while following path')
         end)
   end

   local speed = radiant.entities.get_attribute(entity, 'speed')   
   if speed == nil then
      speed = 1.0
   end
   --Note: Speed is between 10 and 60, normalize to be between 0 and 1
   --TODO: reevaluate, from a design POV, where to put this number
   speed = math.floor(50 + (50 * speed / 60)) / 100

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
   if self._path_trace then
      self._path_trace:destroy()
      self._path_trace = nil
   end
   if self._postures_trace then
      self._postures_trace:destroy()
      self._postures_trace = nil
   end
end

return FollowPathAction